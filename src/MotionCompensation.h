#ifndef MOTION_COMPENSATION_H
#include <algorithm>
#define MOTION_COMPENSATION_H

#include <Arduino.h>
#include "E54_Radar.h"
#include <math.h>

/**
 * Advanced Multi-Anchor Motion Compensation
 *
 * Target Prediction & Interpolation Architecture:
 * 1. Prediction: Uses an Alpha-Beta filter to maintain an internal "world" model of each target,
 *    updating both its Cartesian coordinates (x, y) and velocities (velX, velY).
 * 2. Stabilization (Sensor Motion Rejection): Evaluates the relative distance and geometry between
 *    all relatively stationary targets (anchors). By finding the rigid transformation (translation and
 *    rotation) that maps the current sensor frame to the predicted world frame, it subtracts bulk
 *    platform movement (both translation and twist/yaw) without expensive trig operations.
 * 3. Exposure: The resulting `compensated` coordinates and the exposed `getTargetVelX/Y` can be used
 *    by the UI layer to draw predictive, smooth interpolations between frames instead of linear snapping.
 */
class MotionCompensation {
public:
    friend void test_initialization();
    friend void test_steady_state();
    friend void test_high_acceleration();

    struct TargetState {
        bool active;
        bool isAnchor;
        float x;
        float y;
        float velX;
        float velY;
        float accX;
        float accY;

        // Drop-out resilience
        uint8_t framesLost;

        // Historical trajectory buffer for MeshFlow-inspired optimization
        static const int HISTORY_SIZE = 30;
        float historyX[HISTORY_SIZE];
        float historyY[HISTORY_SIZE];
        int historyHead;
        int historyCount;
        float stdDev;
    };

    MotionCompensation() {}

    void init() {
        for (int i = 0; i < 3; i++) {
            state[i].active = false;
            state[i].isAnchor = false;
            state[i].x = 0; state[i].y = 0;
            state[i].velX = 0; state[i].velY = 0;
            state[i].accX = 0; state[i].accY = 0;
            state[i].framesLost = 0;
            state[i].historyHead = 0;
            state[i].historyCount = 0;
            state[i].stdDev = 0.0f;
        }
        frameCount = 0;
        baseSmoothingAlpha = 0.3; // Default
        baseSmoothingBeta = 0.05; // Default for velocity
        baseSmoothingGamma = 0.005; // Default for acceleration (Alpha-Beta-Gamma filter)
    }

    void setAveragingStrength(int level) {
        // level 1 to 10. Lower alpha = more smoothing
        baseSmoothingAlpha = 1.0 - (level * 0.08);
        if (baseSmoothingAlpha < 0.1) baseSmoothingAlpha = 0.1;
        baseSmoothingBeta = baseSmoothingAlpha * 0.2;
        baseSmoothingGamma = baseSmoothingBeta * 0.1;
    }

    void process(float dt, const RadarTarget targets[3], RadarTarget compensated[3]) {

        float P_x[3], P_y[3];
        predictStates(dt, targets, P_x, P_y);

        int tempAnchorIndices[3];
        int numAnchors = identifyAnchors(targets, tempAnchorIndices);

        int anchorIndices[3];
        numAnchors = validateAnchors(targets, P_x, P_y, tempAnchorIndices, numAnchors, anchorIndices);

        float Cp_x = 0, Cp_y = 0;
        float Cq_x = 0, Cq_y = 0;
        float cosT = 1.0f, sinT = 0.0f;
        calculateTransform(targets, P_x, P_y, anchorIndices, numAnchors, Cp_x, Cp_y, Cq_x, Cq_y, cosT, sinT);

        // 2D Parametric Filtering: Apply Exponential Moving Average (EMA) to global rotational motion vectors.
        // This separates active camera panning from high-frequency shakes as described in Section 2.1.2 of video stabilization literature.
        float filterGain = 0.3f; // Lower = smoother, higher = more responsive to active movement
        emaCosT = (1.0f - filterGain) * emaCosT + filterGain * cosT;
        emaSinT = (1.0f - filterGain) * emaSinT + filterGain * sinT;

        // Re-normalize the filtered quaternion/vector to prevent shrinking
        float mag = sqrtf(emaCosT * emaCosT + emaSinT * emaSinT);
        if (mag > 0.01f) {
            float invMag = 1.0f / mag;
            emaCosT *= invMag;
            emaSinT *= invMag;
        } else {
            emaCosT = 1.0f;
            emaSinT = 0.0f;
        }

        stabilizeAndUpdate(dt, targets, P_x, P_y, numAnchors, Cp_x, Cp_y, Cq_x, Cq_y, emaCosT, emaSinT, compensated);

        frameCount++;
    }

    bool isAnchorValid() const {
        for (int i=0; i<3; i++) {
            if (state[i].active && state[i].isAnchor) return true;
        }
        return false;
    }

    // Expose the global centroid of the anchors for UI debugging (optional)
    int16_t getAnchorX() const {
        float cx = 0; int count = 0;
        for (int i=0; i<3; i++) {
            if (state[i].active && state[i].isAnchor) { cx += state[i].x; count++; }
        }
        return count > 0 ? std::lround(cx / count) : 0;
    }

    int16_t getAnchorY() const {
        float cy = 0; int count = 0;
        for (int i=0; i<3; i++) {
            if (state[i].active && state[i].isAnchor) { cy += state[i].y; count++; }
        }
        return count > 0 ? std::lround(cy / count) : 0;
    }

    // Expose velocity and state data for advanced prediction UI interpolation
    bool isTargetActive(int i) const { return state[i].active; }
    float getTargetVelX(int i) const { return state[i].velX; }
    float getTargetVelY(int i) const { return state[i].velY; }
    float getTargetAccX(int i) const { return state[i].accX; }
    float getTargetAccY(int i) const { return state[i].accY; }
    float getTargetStdDev(int i) const { return state[i].stdDev; }

    void forceReset() { init(); }
    int runSelfTest() {
        MotionCompensation testMc;
        testMc.init();
        RadarTarget target = {true, 1000, 2000, 10, 50, false};
        testMc.updateFilterState(0, 0.1f, 1000.0f, 2000.0f, 1000.0f, 2000.0f, target);
        if (testMc.state[0].active != true || testMc.state[0].x != 1000.0f) {
            return 1;
        }
        return 0;
    }

private:

    void predictStates(float dt, const RadarTarget targets[3], float P_x[3], float P_y[3]) {
        float dt_sq_half = (dt * dt) * 0.5f;
        for (int i = 0; i < 3; i++) {
            if (state[i].active) {
                P_x[i] = state[i].x + state[i].velX * dt + state[i].accX * dt_sq_half;
                P_y[i] = state[i].y + state[i].velY * dt + state[i].accY * dt_sq_half;
            } else {
                P_x[i] = targets[i].x;
                P_y[i] = targets[i].y;
            }
        }
    }

    int identifyAnchors(const RadarTarget targets[3], int tempAnchorIndices[3]) {
        int numAnchors = 0;
        for (int i = 0; i < 3; i++) {
            if (targets[i].active && state[i].active) {
                int16_t absSpeed = abs(targets[i].speed);
                if (absSpeed < STATIC_SPEED_INITIAL_THRESHOLD * 2) {
                    tempAnchorIndices[numAnchors++] = i;
                    state[i].isAnchor = true;
                } else {
                    state[i].isAnchor = false;
                }
            } else {
                state[i].isAnchor = false;
            }
        }
        return numAnchors;
    }

    int validateAnchors(const RadarTarget targets[3], const float P_x[3], const float P_y[3], const int tempAnchorIndices[3], int numAnchors, int anchorIndices[3]) {
        int validatedAnchors = 0;

        if (numAnchors == 3) {
            float err[3] = {0, 0, 0};
            for (int i = 0; i < 3; i++) {
                int a = tempAnchorIndices[i];
                for (int j = i + 1; j < 3; j++) {
                    int b = tempAnchorIndices[j];
                    float dQx = targets[a].x - targets[b].x;
                    float dQy = targets[a].y - targets[b].y;
                    float distQSq = dQx*dQx + dQy*dQy;

                    float dPx = P_x[a] - P_x[b];
                    float dPy = P_y[a] - P_y[b];
                    float distPSq = dPx*dPx + dPy*dPy;

                    float diff = fabsf(distQSq - distPSq) / (distPSq + 1.0f);
                    err[i] += diff;
                    err[j] += diff;
                }
            }

            float minErr = std::min(err[0], std::min(err[1], err[2]));
            for (int i = 0; i < 3; i++) {
                if (err[i] < 0.3f || err[i] <= (minErr * 2.0f + 0.1f)) {
                    anchorIndices[validatedAnchors++] = tempAnchorIndices[i];
                } else {
                    state[tempAnchorIndices[i]].isAnchor = false;
                }
            }
        } else if (numAnchors == 2) {
            int a = tempAnchorIndices[0];
            int b = tempAnchorIndices[1];
            float dQx = targets[a].x - targets[b].x;
            float dQy = targets[a].y - targets[b].y;
            float dPx = P_x[a] - P_x[b];
            float dPy = P_y[a] - P_y[b];

            float diff = fabsf((dQx*dQx + dQy*dQy) - (dPx*dPx + dPy*dPy)) / ((dPx*dPx + dPy*dPy) + 1.0f);
            if (diff < 0.2f) {
                anchorIndices[0] = a;
                anchorIndices[1] = b;
                validatedAnchors = 2;
            } else {
                state[a].isAnchor = false;
                state[b].isAnchor = false;
            }
        } else if (numAnchors == 1) {
            anchorIndices[0] = tempAnchorIndices[0];
            validatedAnchors = 1;
        }

        return validatedAnchors;
    }

    void calculateTransform(const RadarTarget targets[3], const float P_x[3], const float P_y[3], const int anchorIndices[3], int numAnchors, float& Cp_x, float& Cp_y, float& Cq_x, float& Cq_y, float& cosT, float& sinT) {
        if (numAnchors > 0) {
            for (int k = 0; k < numAnchors; k++) {
                int i = anchorIndices[k];
                Cp_x += P_x[i];
                Cp_y += P_y[i];
                Cq_x += targets[i].x;
                Cq_y += targets[i].y;
            }
            Cp_x /= numAnchors;
            Cp_y /= numAnchors;
            Cq_x /= numAnchors;
            Cq_y /= numAnchors;

            if (numAnchors >= 2) {
                float S = 0, C = 0;
                for (int k = 0; k < numAnchors; k++) {
                    int i = anchorIndices[k];
                    float vPx = P_x[i] - Cp_x;
                    float vPy = P_y[i] - Cp_y;
                    float vQx = targets[i].x - Cq_x;
                    float vQy = targets[i].y - Cq_y;
                    S += (vPx * vQy - vPy * vQx);
                    C += (vPx * vQx + vPy * vQy);
                }
                float M = sqrtf(C * C + S * S);
                if (M > 1.0f) {
                    float invM = 1.0f / M;
                    cosT = C * invM;
                    sinT = S * invM;
                }
            }
        }
    }

    void stabilizeAndUpdate(float dt, const RadarTarget targets[3], const float P_x[3], const float P_y[3], int numAnchors, float Cp_x, float Cp_y, float Cq_x, float Cq_y, float cosT, float sinT, RadarTarget compensated[3]) {
        for (int i = 0; i < 3; i++) {
            compensated[i] = targets[i];

            if (targets[i].active) {
                float qx = targets[i].x;
                float qy = targets[i].y;

                float stab_x = qx;
                float stab_y = qy;

                if (numAnchors > 0) {
                    float vQx = qx - Cq_x;
                    float vQy = qy - Cq_y;
                    float vQx_rot = vQx * cosT + vQy * sinT;
                    float vQy_rot = -vQx * sinT + vQy * cosT;
                    stab_x = Cp_x + vQx_rot;
                    stab_y = Cp_y + vQy_rot;
                }

                // Mahalanobis Gating (Handheld Ghost Rejection)
                if (state[i].active) {
                    float maxAllowedDist = 3000.0f; // 3 meters fallback
                    // A person running at 10m/s (olympic sprinter) could cover 1m in 0.1s.
                    // We gate at 1.5 * max theoretical speed distance for the given dt
                    float maxSpeedAllowed = 15000.0f; // mm/s
                    maxAllowedDist = maxSpeedAllowed * dt;
                    // Add a base noise floor to prevent over-gating when dt is extremely small (e.g. fast consecutive frames)
                    float baseNoiseFloor = 200.0f; // mm (20cm minimum gate radius)
                    if (maxAllowedDist < baseNoiseFloor) {
                        maxAllowedDist = baseNoiseFloor;
                    }

                    float dx = stab_x - state[i].x;
                    float dy = stab_y - state[i].y;
                    float distMovedSq = dx*dx + dy*dy;

                    // If target dropped out briefly but reappears within gating distance, reset drop counter
                    state[i].framesLost = 0;

                    if (distMovedSq > maxAllowedDist * maxAllowedDist) {
                        // Reject this target frame (likely a teleporting ghost)
                        // Hold position using predicted coordinates
                        stab_x = P_x[i];
                        stab_y = P_y[i];
                    }
                }

                compensated[i].x = std::lround(stab_x);
                compensated[i].y = std::lround(stab_y);
                compensated[i].active = true;
                compensated[i].isCoasting = false;

                updateFilterState(i, dt, stab_x, stab_y, P_x[i], P_y[i], targets[i]);
            } else {
                // Drop-out resilience logic
                const uint8_t maxFramesLost = 10;
                if (state[i].active && state[i].framesLost < maxFramesLost) {
                    state[i].framesLost++;
                    // Predict forward using last known velocity
                    float px = P_x[i];
                    float py = P_y[i];

                    // Update state to use predicted coordinates to coast through dropouts
                    state[i].x = px;
                    state[i].y = py;

                    // Provide a synthetic compensated target to UI
                    compensated[i].x = std::lround(px);
                    compensated[i].y = std::lround(py);
                    compensated[i].speed = std::lround(sqrtf(state[i].velX*state[i].velX + state[i].velY*state[i].velY));
                    compensated[i].active = true;
                    compensated[i].resolution = 0; // indicates interpolated frame
                    compensated[i].isCoasting = true;
                } else {
                    // Permanently drop after grace period
                    state[i].active = false;
                    state[i].isAnchor = false;
                    compensated[i].active = false;
                    compensated[i].isCoasting = false;
                }
            }
        }
    }

    void updateFilterState(int i, float dt, float stab_x, float stab_y, float px, float py, const RadarTarget& rawTarget) {
        if (!state[i].active) {
            state[i].x = stab_x;
            state[i].y = stab_y;
            state[i].velX = 0;
            state[i].velY = 0;
            state[i].accX = 0;
            state[i].accY = 0;
            state[i].active = true;
            state[i].framesLost = 0;
            state[i].historyCount = 0;
            state[i].stdDev = 0.0f;
            state[i].historyHead = 0;
        } else {
            // Apply MeshFlow-inspired Gaussian window smoothing using historical data
            float smoothX = stab_x;
            float smoothY = stab_y;

            if (state[i].historyCount > 0) {
                float totalWeight = 1.0f; // Weight for the new coordinate
                float sumX = stab_x * 1.0f;
                float sumY = stab_y * 1.0f;

                // Velocity-Adaptive Gaussian Window Size
                // As per literature on temporal video stabilization (MeshFlow real-time adaptation),
                // using a wide smoothing window induces heavy lag on moving objects.
                // We dynamically scale the window size based on current velocity.
                // The sensor natively outputs speed in cm/s. We convert to mm/s for scaling logic.
                float currentSpeed = fabsf((float)rawTarget.speed * 10.0f);
                float maxSpeed = 3000.0f; // 3 m/s (3000 mm/s) as upper bound for fast motion

                // If stationary, use full history. If moving fast, collapse to very small window.
                float window_size = (float)state[i].historyCount;
                if (currentSpeed > 50.0f) { // Only shrink if moving more than noise floor
                    float speedFactor = 1.0f - (currentSpeed / maxSpeed);
                    if (speedFactor < 0.1f) speedFactor = 0.1f;
                    window_size = window_size * speedFactor;
                }

                if (window_size > 1.0f) {
                    float meanX = 0;
                    float meanY = 0;

                    // Optimization: Precompute inverse window size squared to replace expensive FPU division in the loop
                    float invWindowSq = -9.0f / (window_size * window_size);

                    for (int j = 0; j < state[i].historyCount; j++) {
                        // Calculate index in circular buffer
                        int histIdx = (state[i].historyHead - 1 - j + TargetState::HISTORY_SIZE) % TargetState::HISTORY_SIZE;

                        float hX = state[i].historyX[histIdx];
                        float hY = state[i].historyY[histIdx];

                        meanX += hX;
                        meanY += hY;

                        // Time delta `r-t` corresponds to `j + 1` (how many frames ago)
                        float r_t = (float)(j + 1);

                        // To heavily weight recent frames when the window is small (moving target),
                        // we also apply a baseline exponential time decay factor.
                        float weight = expf((r_t * r_t) * invWindowSq);

                        // Only add weight if it is statistically significant (speeds up loop and rejects stale data for fast targets)
                        if (weight > 0.05f) {
                            sumX += hX * weight;
                            sumY += hY * weight;
                            totalWeight += weight;
                        }
                    }

                    // Optimization: Use reciprocal multiplication instead of division
                    float invTotalWeight = 1.0f / totalWeight;
                    smoothX = sumX * invTotalWeight;
                    smoothY = sumY * invTotalWeight;

                    // Calculate unweighted Standard Deviation (Variance spread) of the coordinate history buffer for UI
                    float invCount = 1.0f / (float)state[i].historyCount;
                    meanX *= invCount;
                    meanY *= invCount;
                    float varSum = 0;
                    for (int j = 0; j < state[i].historyCount; j++) {
                         int histIdx = (state[i].historyHead - 1 - j + TargetState::HISTORY_SIZE) % TargetState::HISTORY_SIZE;
                         float dx = state[i].historyX[histIdx] - meanX;
                         float dy = state[i].historyY[histIdx] - meanY;
                         varSum += (dx*dx + dy*dy);
                    }
                    state[i].stdDev = sqrtf(varSum * invCount);
                }
            }

            float alpha = baseSmoothingAlpha;
            float beta = baseSmoothingBeta;
            float gamma = baseSmoothingGamma;

            // Residual is calculated against the smoothed objective, not the raw stabilized point
            float residualX = smoothX - px;
            float residualY = smoothY - py;
            float distSq = residualX * residualX + residualY * residualY;

            // Adaptive Gain Selection
            float accMagnitudeSq = state[i].accX * state[i].accX + state[i].accY * state[i].accY;
            float accThresholdSq = 4000000.0f; // e.g., 2000 mm/s^2 squared

            if (accMagnitudeSq > accThresholdSq) {
                // High acceleration -> prioritize Real-Time Tracking
                alpha = std::min(1.0f, alpha * 2.5f);
                beta = alpha * 0.4f;
                gamma = alpha * 0.1f;
            } else if (state[i].isAnchor) {
                // Stable/Anchor -> Lock In position
                if (distSq > 10000.0f) {
                    alpha *= 0.2f; beta *= 0.1f; gamma *= 0.1f;
                } else if (distSq < 400.0f) {
                    alpha = std::min(1.0f, alpha * 1.5f);
                    beta *= 1.5f;
                    gamma *= 1.5f;
                }
            } else {
                alpha = std::min(1.0f, alpha * 1.5f);
                beta = alpha * 0.2f;
                gamma = alpha * 0.05f;
            }

            // Resolution weighting
            // High resolution (low value) = high confidence = high gain
            if (rawTarget.resolution > 0) {
                 float resWeight = 250.0f / (float)rawTarget.resolution;
                 resWeight = std::min(std::max(resWeight, 0.5f), 2.0f);
                 alpha = std::min(1.0f, alpha * resWeight);
                 beta *= resWeight;
                 gamma *= resWeight;
            }

            state[i].x = px + alpha * residualX;
            state[i].y = py + alpha * residualY;
            float invDt = 1.0f / dt;
            state[i].velX = state[i].velX + (beta * residualX * invDt);
            state[i].velY = state[i].velY + (beta * residualY * invDt);
            float dt_sq_half = (dt * dt) * 0.5f;
            float invDtSqHalf = 1.0f / dt_sq_half;
            state[i].accX = state[i].accX + (gamma * residualX * invDtSqHalf);
            state[i].accY = state[i].accY + (gamma * residualY * invDtSqHalf);

            float maxAcc = 5000.0f;
            state[i].accX = std::min(std::max(state[i].accX, -maxAcc), maxAcc);
            state[i].accY = std::min(std::max(state[i].accY, -maxAcc), maxAcc);

            // Update circular history buffer with final determined state
            state[i].historyX[state[i].historyHead] = state[i].x;
            state[i].historyY[state[i].historyHead] = state[i].y;
            state[i].historyHead = (state[i].historyHead + 1) % TargetState::HISTORY_SIZE;
            if (state[i].historyCount < TargetState::HISTORY_SIZE) {
                state[i].historyCount++;
            }
        }
    }


    TargetState state[3];
    uint32_t frameCount;
    float baseSmoothingAlpha;
    float baseSmoothingBeta;
    float baseSmoothingGamma;

    // Exponential Moving Average state for global motion vectors
    float emaCosT = 1.0f;
    float emaSinT = 0.0f;

    const int16_t STATIC_SPEED_INITIAL_THRESHOLD = 15; // cm/s
};

#endif
