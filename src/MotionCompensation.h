#ifndef MOTION_COMPENSATION_H
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
    struct TargetState {
        bool active;
        bool isAnchor;
        float x;
        float y;
        float velX;
        float velY;
        float accX;
        float accY;
    };

    MotionCompensation() {}

    void init() {
        for (int i = 0; i < 3; i++) {
            state[i] = {false, false, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
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

    void process(RadarTarget targets[3], RadarTarget compensated[3]) {
        float dt = 0.1f; // Assuming ~10Hz update rate

        // 1. Predict next state for all active tracked targets (Alpha-Beta-Gamma curve prediction)
        float P_x[3], P_y[3];
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

        // 2. Identify Anchors (stationary reference points)
        int numAnchors = 0;
        int tempAnchorIndices[3];
        for (int i = 0; i < 3; i++) {
            if (targets[i].active && state[i].active) {
                // Must have been active in previous frame to have a valid prediction (P_x, P_y)
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

        // Dynamic Anchor Validation (Cross-Target Rigidity Rejection)
        // If an object is moving at the exact same speed as the sensor, it might have zero relative speed,
        // falsely flagging it as an anchor. We reject anchors whose relative distances to other anchors
        // vary wildly compared to the predicted world state.
        int anchorIndices[3];
        int validatedAnchors = 0;

        if (numAnchors == 3) {
            // Check rigidity of the triangle. If one point is moving relative to the others, drop it.
            float err[3] = {0, 0, 0}; // Error score for each anchor
            for (int i = 0; i < 3; i++) {
                int a = tempAnchorIndices[i];
                for (int j = i + 1; j < 3; j++) {
                    int b = tempAnchorIndices[j];
                    // Raw relative distance squared
                    float dQx = targets[a].x - targets[b].x;
                    float dQy = targets[a].y - targets[b].y;
                    float distQSq = dQx*dQx + dQy*dQy;

                    // Predicted world relative distance squared
                    float dPx = P_x[a] - P_x[b];
                    float dPy = P_y[a] - P_y[b];
                    float distPSq = dPx*dPx + dPy*dPy;

                    // Ratio of change. If lengths change by > ~10%, it's not rigid.
                    float diff = fabsf(distQSq - distPSq) / (distPSq + 1.0f);
                    err[i] += diff;
                    err[j] += diff;
                }
            }

            // Find if there's an anomalous anchor (e.g. one error is much higher)
            float minErr = min(err[0], min(err[1], err[2]));
            for (int i = 0; i < 3; i++) {
                // If error is relatively low, or close to the minimum (for the 2 stationary ones), keep it
                if (err[i] < 0.3f || err[i] <= (minErr * 2.0f + 0.1f)) {
                    anchorIndices[validatedAnchors++] = tempAnchorIndices[i];
                } else {
                    state[tempAnchorIndices[i]].isAnchor = false;
                }
            }
        } else if (numAnchors == 2) {
            // Check the line segment. If length changes drastically, drop both (can't tell which is wrong without 3rd reference)
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

        numAnchors = validatedAnchors;

        // 3. Calculate Global Rigid Transformation (Kabsch algorithm in 2D)
        // Maps the current raw points (Q) to the predicted world points (P)
        float Cp_x = 0, Cp_y = 0;
        float Cq_x = 0, Cq_y = 0;
        float cosT = 1.0f, sinT = 0.0f;

        if (numAnchors > 0) {
            // Find Centroids
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

            // Find Rotation if 2 or more anchors are available
            if (numAnchors >= 2) {
                float S = 0, C = 0;
                for (int k = 0; k < numAnchors; k++) {
                    int i = anchorIndices[k];
                    float vPx = P_x[i] - Cp_x;
                    float vPy = P_y[i] - Cp_y;
                    float vQx = targets[i].x - Cq_x;
                    float vQy = targets[i].y - Cq_y;
                    S += (vPx * vQy - vPy * vQx); // Cross-product sum
                    C += (vPx * vQx + vPy * vQy); // Dot-product sum
                }
                float M = sqrtf(C * C + S * S);
                if (M > 1.0f) {
                    cosT = C / M;
                    sinT = S / M;
                }
            }
        }

        // 4. Stabilize targets and update internal state
        for (int i = 0; i < 3; i++) {
            compensated[i] = targets[i]; // Copy attributes (active, speed, resolution)

            if (targets[i].active) {
                float qx = targets[i].x;
                float qy = targets[i].y;

                float stab_x = qx;
                float stab_y = qy;

                if (numAnchors > 0) {
                    // Apply inverse transform to stabilize target
                    float vQx = qx - Cq_x;
                    float vQy = qy - Cq_y;
                    float vQx_rot = vQx * cosT + vQy * sinT;
                    float vQy_rot = -vQx * sinT + vQy * cosT;
                    stab_x = Cp_x + vQx_rot;
                    stab_y = Cp_y + vQy_rot;
                }

                compensated[i].x = (int16_t)stab_x;
                compensated[i].y = (int16_t)stab_y;

                // 5. Update Alpha-Beta-Gamma filter state
                if (!state[i].active) {
                    // Newly tracked target
                    state[i].x = stab_x;
                    state[i].y = stab_y;
                    state[i].velX = 0;
                    state[i].velY = 0;
                    state[i].accX = 0;
                    state[i].accY = 0;
                    state[i].active = true;
                } else {
                    float alpha = baseSmoothingAlpha;
                    float beta = baseSmoothingBeta;
                    float gamma = baseSmoothingGamma;

                    float residualX = stab_x - P_x[i];
                    float residualY = stab_y - P_y[i];
                    float distSq = residualX * residualX + residualY * residualY;

                    if (state[i].isAnchor) {
                        if (distSq > 10000.0f) {
                            alpha *= 0.2f; beta *= 0.1f; gamma *= 0.1f; // High jitter, distrust measurement
                        } else if (distSq < 400.0f) {
                            alpha = min(1.0f, alpha * 1.5f); // Smooth, trust more
                            beta *= 1.5f;
                            gamma *= 1.5f;
                        }
                    } else {
                        // Dynamic targets need faster tracking response
                        alpha = min(1.0f, alpha * 2.0f);
                        beta = alpha * 0.2f;
                        gamma = alpha * 0.05f;
                    }

                    state[i].x = P_x[i] + alpha * residualX;
                    state[i].y = P_y[i] + alpha * residualY;
                    state[i].velX = state[i].velX + (beta * residualX / dt);
                    state[i].velY = state[i].velY + (beta * residualY / dt);
                    float dt_sq_half = (dt * dt) * 0.5f;
                    state[i].accX = state[i].accX + (gamma * residualX / dt_sq_half);
                    state[i].accY = state[i].accY + (gamma * residualY / dt_sq_half);

                    // Constrain acceleration to prevent runaway curves
                    float maxAcc = 5000.0f; // mm/s^2 (5 m/s^2)
                    if (state[i].accX > maxAcc) state[i].accX = maxAcc;
                    if (state[i].accX < -maxAcc) state[i].accX = -maxAcc;
                    if (state[i].accY > maxAcc) state[i].accY = maxAcc;
                    if (state[i].accY < -maxAcc) state[i].accY = -maxAcc;
                }
            } else {
                state[i].active = false;
                state[i].isAnchor = false;
            }
        }
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
        return count > 0 ? (int16_t)(cx / count) : 0;
    }

    int16_t getAnchorY() const {
        float cy = 0; int count = 0;
        for (int i=0; i<3; i++) {
            if (state[i].active && state[i].isAnchor) { cy += state[i].y; count++; }
        }
        return count > 0 ? (int16_t)(cy / count) : 0;
    }

    // Expose velocity and state data for advanced prediction UI interpolation
    bool isTargetActive(int i) const { return state[i].active; }
    float getTargetVelX(int i) const { return state[i].velX; }
    float getTargetVelY(int i) const { return state[i].velY; }
    float getTargetAccX(int i) const { return state[i].accX; }
    float getTargetAccY(int i) const { return state[i].accY; }

    void forceReset() { init(); }

private:
    TargetState state[3];
    uint32_t frameCount;
    float baseSmoothingAlpha;
    float baseSmoothingBeta;
    float baseSmoothingGamma;

    const int16_t STATIC_SPEED_INITIAL_THRESHOLD = 15; // cm/s
};

#endif
