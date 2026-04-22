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
        smoothingAlpha = 0.3; // Default
    }

    void setAveragingStrength(int level) {
        // level 1 to 10
        // lower alpha means more smoothing
        smoothingAlpha = 1.0 - (level * 0.08); // level 5 -> 0.6, level 10 -> 0.2
        if (smoothingAlpha < 0.1) smoothingAlpha = 0.1;
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
                anchorID = anchorIdx; // Track the physical slot if we want to, though IDs are not persistent in E54.
                framesWithoutAnchor = 0;
                frameCount = 1;
            } else {
                // Smooth anchor position
                // Since this is the anchor moving *relative to us* due to handheld jitter, we update our
                // "reference origin" to track its low-frequency movement but filter out high-frequency jitter.
                // However, the anchor is a physical object. If we move the device smoothly, the anchor shifts smoothly in our coordinates.
                anchorX = (int16_t)((1.0 - smoothingAlpha) * anchorX + smoothingAlpha * currentX);
                anchorY = (int16_t)((1.0 - smoothingAlpha) * anchorY + smoothingAlpha * currentY);
                framesWithoutAnchor = 0;
                frameCount++;
            }

            // The jitter is the difference between current perceived anchor position and its filtered position
            int16_t jitterX = currentX - anchorX;
            int16_t jitterY = currentY - anchorY;

            // 2. Compensate other targets
            for (int i = 0; i < 3; i++) {
                if (i != anchorIdx && targets[i].active) {
                    // Subtract the jitter from the dynamic targets
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
    float smoothingAlpha;

    // Thresholds
    const int16_t STATIC_SPEED_INITIAL_THRESHOLD = 15; // cm/s
    const int16_t MAX_ANCHOR_MOVEMENT_PER_FRAME = 800; // mm

    int findAnchorIndex(RadarTarget targets[3]) {
        int bestIdx = -1;
        int16_t minSpeed = 32767;

        for (int i = 0; i < 3; i++) {
            if (targets[i].active) {
                int16_t absSpeed = abs(targets[i].speed);

                if (anchorValid) {
                    // If we already have an anchor, the anchor is allowed to have speed relative to us
                    // (because we might be moving the handheld device towards/away from the anchor wall).
                    // The main criteria is that it shouldn't teleport wildly from its last known position.
                    int16_t dx = targets[i].x - anchorX;
                    int16_t dy = targets[i].y - anchorY;
                    if (sqrt(dx*dx + dy*dy) < MAX_ANCHOR_MOVEMENT_PER_FRAME) {
                        bestIdx = i; // Prefer keeping the one close to our existing anchor
                        break; // Stop looking, we found our tracked anchor
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
