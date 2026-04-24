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
        anchorID = -1;
        framesWithoutAnchor = 0;
        frameCount = 0;
        baseSmoothingAlpha = 0.3; // Default
    }

    void setAveragingStrength(int level) {
        // level 1 to 10
        // lower alpha means more smoothing (slower tracking)
        baseSmoothingAlpha = 1.0 - (level * 0.08);
        if (baseSmoothingAlpha < 0.1) baseSmoothingAlpha = 0.1;
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

            if (!anchorValid) {
                // Initialize anchor position
                anchorX = currentX;
                anchorY = currentY;
                anchorValid = true;
                anchorID = anchorIdx;
                framesWithoutAnchor = 0;
                frameCount = 1;
            } else {
                // Adapt the smoothing alpha based on the derivative/error of the movement,
                // inspired by the Anchor-Based Compensation paper's adaptive weight estimation (Eq 5).
                //
                // If the anchor jumps rapidly (high derivative -> transient jitter/shake), we lower the alpha
                // to rely more on the historical position (heavy smoothing).
                // If the anchor moves slowly/smoothly (low derivative -> deliberate panning of device),
                // we increase the alpha to let the anchor "catch up" and not lag behind.

                float dx = (float)(currentX - anchorX);
                float dy = (float)(currentY - anchorY);
                float distSq = (dx * dx) + (dy * dy);

                // Calculate adaptive alpha
                // We use squared distances to avoid expensive sqrt() calls.
                // 100mm -> 10000 sq mm
                // 20mm  -> 400 sq mm

                float adaptiveAlpha = baseSmoothingAlpha;

                if (distSq > 10000.0f) {
                    // Sharp transient motion: Heavily distrust the new measurement
                    adaptiveAlpha = baseSmoothingAlpha * 0.2f;
                } else if (distSq < 400.0f) {
                    // Smooth tracking: Trust the measurement more to prevent drift lagging
                    adaptiveAlpha = baseSmoothingAlpha * 1.5f;
                    if (adaptiveAlpha > 1.0f) adaptiveAlpha = 1.0f;
                }

                // Apply adaptive Exponential Moving Average (EMA)
                anchorX = (int16_t)((1.0f - adaptiveAlpha) * anchorX + adaptiveAlpha * (float)currentX);
                anchorY = (int16_t)((1.0f - adaptiveAlpha) * anchorY + adaptiveAlpha * (float)currentY);

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
    int16_t anchorX;
    int16_t anchorY;
    int anchorID;
    int framesWithoutAnchor;
    uint32_t frameCount;
    float baseSmoothingAlpha;

    // Thresholds
    const int16_t STATIC_SPEED_INITIAL_THRESHOLD = 15; // cm/s
    const float MAX_ANCHOR_MOVEMENT_PER_FRAME_SQ = 640000.0f; // 800 * 800

    int findAnchorIndex(RadarTarget targets[3]) {
        int bestIdx = -1;
        int16_t minSpeed = 32767;

        for (int i = 0; i < 3; i++) {
            if (targets[i].active) {
                int16_t absSpeed = abs(targets[i].speed);

                if (anchorValid) {
                    // If we already have an anchor, verify it hasn't teleported wildly
                    float dx = (float)(targets[i].x - anchorX);
                    float dy = (float)(targets[i].y - anchorY);
                    if ((dx*dx + dy*dy) < MAX_ANCHOR_MOVEMENT_PER_FRAME_SQ) {
                        bestIdx = i;
                        break;
                    }
                } else {
                    // Looking for a NEW anchor. It must be relatively static right now.
                    if (absSpeed < STATIC_SPEED_INITIAL_THRESHOLD && absSpeed < minSpeed) {
                        bestIdx = i;
                        minSpeed = absSpeed;
                    }
                }
            }
        }
        return bestIdx;
    }
};

#endif
