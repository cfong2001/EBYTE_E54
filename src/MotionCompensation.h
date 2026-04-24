#ifndef MOTION_COMPENSATION_H
#define MOTION_COMPENSATION_H

#include <Arduino.h>
#include "E54_Radar.h"
#include <math.h>

class MotionCompensation {
public:
    MotionCompensation() {}

    void init() {
        anchorValid = false;
        anchorX = 0;
        anchorY = 0;
        anchorVelX = 0;
        anchorVelY = 0;
        anchorID = -1;
        framesWithoutAnchor = 0;
        frameCount = 0;
        baseSmoothingAlpha = 0.3; // Default
        baseSmoothingBeta = 0.05; // Default for velocity
    }

    void setAveragingStrength(int level) {
        // level 1 to 10
        // lower alpha means more smoothing (slower tracking)
        baseSmoothingAlpha = 1.0 - (level * 0.08);
        if (baseSmoothingAlpha < 0.1) baseSmoothingAlpha = 0.1;
        baseSmoothingBeta = baseSmoothingAlpha * 0.2; // Beta scales with alpha
    }

    // Process targets, identify an anchor if we don't have one, and compensate dynamic targets
    void process(RadarTarget targets[3], RadarTarget compensated[3]) {
        // Copy original to compensated initially
        for (int i = 0; i < 3; i++) {
            compensated[i] = targets[i];
        }

        // 1. Identify or Update Anchor
        int anchorIdx = findAnchorIndex(targets);

        if (anchorIdx != -1) {
            // We have a target that qualifies as an anchor
            int16_t currentX = targets[anchorIdx].x;
            int16_t currentY = targets[anchorIdx].y;

            if (!anchorValid || anchorID != anchorIdx) {
                // Initialize or switch anchor position
                anchorX = currentX;
                anchorY = currentY;
                anchorVelX = 0.0f;
                anchorVelY = 0.0f;
                anchorValid = true;
                anchorID = anchorIdx;
                framesWithoutAnchor = 0;
                frameCount = 1;
            } else {
                // Alpha-Beta Predictive Filtering

                // 1. Predict next state
                float dt = 0.1f; // Assuming ~10Hz update rate
                float predictedX = anchorX + (anchorVelX * dt);
                float predictedY = anchorY + (anchorVelY * dt);

                // 2. Calculate Innovation (Error)
                float residualX = (float)currentX - predictedX;
                float residualY = (float)currentY - predictedY;
                float distSq = (residualX * residualX) + (residualY * residualY);

                // Calculate adaptive alpha and beta
                float adaptiveAlpha = baseSmoothingAlpha;
                float adaptiveBeta = baseSmoothingBeta;

                if (distSq > 10000.0f) {
                    // Sharp transient motion: Heavily distrust the new measurement
                    adaptiveAlpha = baseSmoothingAlpha * 0.2f;
                    adaptiveBeta = baseSmoothingBeta * 0.1f;
                } else if (distSq < 400.0f) {
                    // Smooth tracking: Trust the measurement more to prevent drift lagging
                    adaptiveAlpha = baseSmoothingAlpha * 1.5f;
                    if (adaptiveAlpha > 1.0f) adaptiveAlpha = 1.0f;
                    adaptiveBeta = baseSmoothingBeta * 1.5f;
                }

                // 3. Update state
                anchorX = predictedX + (adaptiveAlpha * residualX);
                anchorY = predictedY + (adaptiveAlpha * residualY);

                // Update velocity
                anchorVelX = anchorVelX + (adaptiveBeta * residualX / dt);
                anchorVelY = anchorVelY + (adaptiveBeta * residualY / dt);

                framesWithoutAnchor = 0;
                frameCount++;
            }

            // The jitter is the difference between current perceived raw anchor position and its adaptively filtered position
            int16_t jitterX = currentX - anchorX;
            int16_t jitterY = currentY - anchorY;

            // 2. Compensate other targets
            for (int i = 0; i < 3; i++) {
                if (i != anchorIdx && targets[i].active) {
                    // Subtract the high-frequency jitter from the dynamic targets
                    compensated[i].x = targets[i].x - jitterX;
                    compensated[i].y = targets[i].y - jitterY;
                }
            }
        } else {
            // No anchor found.
            if (anchorValid) {
                framesWithoutAnchor++;
                // If we lose the anchor for more than 10 frames (about 1 second if updating at 10Hz), reset it
                if (framesWithoutAnchor > 10) {
                    anchorValid = false;
                    anchorID = -1;
                }
            }
        }
    }

    bool isAnchorValid() const { return anchorValid; }
    int16_t getAnchorX() const { return anchorX; }
    int16_t getAnchorY() const { return anchorY; }
    void forceReset() { init(); }

private:
    bool anchorValid;
    float anchorX;
    float anchorY;
    float anchorVelX;
    float anchorVelY;
    int anchorID;
    int framesWithoutAnchor;
    uint32_t frameCount;
    float baseSmoothingAlpha;
    float baseSmoothingBeta;

    // Thresholds
    const int16_t STATIC_SPEED_INITIAL_THRESHOLD = 15; // cm/s
    const float MAX_ANCHOR_MOVEMENT_PER_FRAME_SQ = 640000.0f; // 800 * 800

    int findAnchorIndex(RadarTarget targets[3]) {
        int bestIdx = -1;
        float bestScore = -1.0f;

        for (int i = 0; i < 3; i++) {
            if (targets[i].active) {
                int16_t absSpeed = abs(targets[i].speed);

                // Base requirement for ANY anchor: must be moving relatively slowly
                if (absSpeed > STATIC_SPEED_INITIAL_THRESHOLD * 2) {
                    continue;
                }

                // Calculate a "quality score" for this target to be the anchor.
                // Higher is better.
                // 1. Lower speed is better
                float speedScore = max(0.0f, 1.0f - ((float)absSpeed / (STATIC_SPEED_INITIAL_THRESHOLD * 2.0f)));

                // 2. We prefer targets that have higher resolution (lower resolution value in mm)
                // Assuming typical resolutions are around 200-800mm
                float resScore = 1.0f;
                if (targets[i].resolution > 0) {
                     resScore = 500.0f / (float)targets[i].resolution; // Lower resolution value is better
                     if (resScore > 1.0f) resScore = 1.0f;
                }

                // 3. Continuity bonus: heavily prefer keeping the current anchor if it's still valid
                float continuityBonus = 0.0f;
                if (anchorValid && anchorID == i) {
                    float dx = (float)(targets[i].x - anchorX);
                    float dy = (float)(targets[i].y - anchorY);
                    if ((dx*dx + dy*dy) < MAX_ANCHOR_MOVEMENT_PER_FRAME_SQ) {
                        continuityBonus = 1.5f; // Strong bias to prevent rapid switching
                    } else {
                        continue; // Current anchor teleported, invalidate it
                    }
                }

                float totalScore = (speedScore * 1.0f) + (resScore * 0.5f) + continuityBonus;

                if (totalScore > bestScore) {
                    bestScore = totalScore;
                    bestIdx = i;
                }
            }
        }

        // Return -1 if no target meets even basic criteria
        if (bestScore < 0.5f && !anchorValid) {
            return -1;
        }

        return bestIdx;
    }
};

#endif
