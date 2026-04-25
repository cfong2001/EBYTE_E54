#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <RotaryEncoder.h>
#include <OneButton.h>
#include "E54_Radar.h"

// State of the application
enum AppState {
    STATE_RADAR_VIEW,
    STATE_MENU
};

class UIManager {
public:
    UIManager(TFT_eSPI& display) : tft(display) {
        state = STATE_RADAR_VIEW;
        menuSelection = 0;
        sensitivity = 5;
        locationAveraging = 5;
        interpolationAmount = 0.5f; // New Setting
        actionRequested = 0;

        // Initialize history
        for (int i=0; i<3; i++) {
            lastTargetActive[i] = false;
            targetCurrentX[i] = 120.0f;
            targetCurrentY[i] = 240.0f;
            lastDrawnX[i] = 120;
            lastDrawnY[i] = 240;
        }
    }

    void init() {
        tft.init();
        tft.setRotation(1); // landscape
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        drawRadarBackground();
    }

    void handleEncoder(int dir) {
        if (state == STATE_MENU) {
            menuSelection += dir;
            if (menuSelection < 0) menuSelection = 4;
            if (menuSelection > 4) menuSelection = 0;
            drawMenu();
        } else if (state == STATE_RADAR_VIEW) {
            // Adjust scale or other view parameters if needed in future
        }
    }

    void handleButton() {
        if (state == STATE_RADAR_VIEW) {
            state = STATE_MENU;
            tft.fillScreen(TFT_BLACK);
            drawMenu();
        } else if (state == STATE_MENU) {
            executeMenuAction();
        }
    }

    void setTargetVelocity(int index, float vx, float vy) {
        if (index >= 0 && index < 3) {
            // Convert mm/s or cm/s to screen pixels per second/frame here if needed
            // For now, store it for the predictive interpolation step.
            targetVelX[index] = vx * 120 / 5000;
            targetVelY[index] = -vy * 240 / 5000; // Y is inverted on screen
        }
    }

    // Call this when new radar data arrives (e.g. 10Hz) to set the goal targets
    void updateRadarData(RadarTarget targets[3], bool anchorValid, int16_t anchorX, int16_t anchorY) {
        this->anchorValid = anchorValid;
        this->anchorX = anchorX;
        this->anchorY = anchorY;

        for (int i = 0; i < 3; i++) {
            targetActive[i] = targets[i].active;

            if (targets[i].active) {
                int16_t absSpeed = abs(targets[i].speed);
                if (absSpeed < sensitivity && sensitivity > 1) {
                    targetActive[i] = false;
                } else {
                    targetGoalX[i] = 120 + (targets[i].x * 120 / 5000);
                    targetGoalY[i] = 240 - (targets[i].y * 240 / 5000);

                    if (targetGoalX[i] < 0 || targetGoalX[i] >= 240 || targetGoalY[i] < 0 || targetGoalY[i] >= 240) {
                        targetActive[i] = false; // Off screen
                    }
                }
            }

            // Snap immediately to goal if it just became active
            if (targetActive[i] && !lastTargetActive[i]) {
                targetCurrentX[i] = (float)targetGoalX[i];
                targetCurrentY[i] = (float)targetGoalY[i];
            }
        }
    }

    // Call this frequently in the main loop to handle animations
    void renderLoop() {
        if (state != STATE_RADAR_VIEW) return;

        bool needsRedraw = false;

        // 1. Move targets via interpolation
        for (int i = 0; i < 3; i++) {
            if (targetActive[i]) {
                // To utilize the advanced prediction algorithm (alpha-beta filter velocities)
                // we calculate a predicted goal based on targetVelX/Y over a time step,
                // but since updateRadarData sets the "absolute" targetGoalX/Y from the motion
                // compensated coordinates, we simply smoothly interpolate towards the goal, potentially
                // using the velocity vector to curve or predict the path.

                float diffX = (float)targetGoalX[i] - targetCurrentX[i];
                float diffY = (float)targetGoalY[i] - targetCurrentY[i];

                // If diff is very small, snap it.
                if (abs(diffX) < 0.5f && abs(diffY) < 0.5f) {
                    targetCurrentX[i] = (float)targetGoalX[i];
                    targetCurrentY[i] = (float)targetGoalY[i];
                } else {
                    // Combine standard linear interpolation with predictive velocity feed-forward
                    // The feed-forward term (targetVel) slightly nudges the interpolation in the direction of travel
                    float feedForwardX = targetVelX[i] * 0.05f; // DT ~30ms
                    float feedForwardY = targetVelY[i] * 0.05f;

                    targetCurrentX[i] += (diffX * interpolationAmount) + feedForwardX;
                    targetCurrentY[i] += (diffY * interpolationAmount) + feedForwardY;
                }
            }
        }

        // 2. Check if we actually need to draw something (to save TFT SPI bandwidth)
        for (int i = 0; i < 3; i++) {
            if (targetActive[i]) {
                int currentScreenX = (int)targetCurrentX[i];
                int currentScreenY = (int)targetCurrentY[i];
                if (currentScreenX != lastDrawnX[i] || currentScreenY != lastDrawnY[i]) {
                    needsRedraw = true;
                }
            } else if (lastTargetActive[i]) { // Became inactive
                needsRedraw = true;
            }
        }

        if (!needsRedraw) return;

        // 3. Erase old targets
        for (int i = 0; i < 3; i++) {
            if (lastTargetActive[i]) {
                tft.fillCircle(lastDrawnX[i], lastDrawnY[i], 5, TFT_BLACK);
                tft.setTextColor(TFT_BLACK, TFT_BLACK);
                tft.setCursor(lastDrawnX[i] + 8, lastDrawnY[i] - 8);
                tft.printf("T%d", i + 1);
            }
        }

        // Redraw background lines that might have been erased
        drawRadarBackground();

        // Draw anchor status text
        if (anchorValid) {
            tft.setTextColor(TFT_GREEN, TFT_BLACK);
            tft.setCursor(5, 5);
            tft.printf("Anchor: (%d, %d)   ", anchorX, anchorY);
        } else {
            tft.setTextColor(TFT_RED, TFT_BLACK);
            tft.setCursor(5, 5);
            tft.printf("No Anchor          ");
        }

        // Draw new targets
        for (int i = 0; i < 3; i++) {
            lastTargetActive[i] = targetActive[i];

            if (targetActive[i]) {
                int screenX = (int)targetCurrentX[i];
                int screenY = (int)targetCurrentY[i];

                tft.fillCircle(screenX, screenY, 5, TFT_RED);
                tft.setTextColor(TFT_YELLOW, TFT_BLACK);
                tft.setCursor(screenX + 8, screenY - 8);
                tft.printf("T%d", i + 1);

                lastDrawnX[i] = screenX;
                lastDrawnY[i] = screenY;
            }
        }
    }

    // Getters for settings
    int getSensitivity() { return sensitivity; }
    int getLocationAveraging() { return locationAveraging; }

    int consumeAction() {
        int act = actionRequested;
        actionRequested = 0;
        return act;
    }

private:
    TFT_eSPI& tft;
    AppState state;
    int menuSelection;

    // Settings
    int sensitivity;       // 1-10
    int locationAveraging; // 1-10
    float interpolationAmount; // 0.1f - 1.0f

    int actionRequested; // 1=reset

    // Data model
    bool anchorValid;
    int16_t anchorX;
    int16_t anchorY;

    bool targetActive[3];
    int targetGoalX[3];
    int targetGoalY[3];

    // Rendering history/animation states
    float targetCurrentX[3];
    float targetCurrentY[3];
    float targetVelX[3];
    float targetVelY[3];

    bool lastTargetActive[3];
    int lastDrawnX[3];
    int lastDrawnY[3];

    void drawRadarBackground() {
        // Draw arcs for distance
        tft.drawCircle(120, 240, 60, TFT_DARKGREY);
        tft.drawCircle(120, 240, 120, TFT_DARKGREY);
        tft.drawCircle(120, 240, 180, TFT_DARKGREY);

        // Draw center line
        tft.drawLine(120, 240, 120, 0, TFT_DARKGREY);
        tft.drawLine(120, 240, 0, 120, TFT_DARKGREY);
        tft.drawLine(120, 240, 240, 120, TFT_DARKGREY);
    }

    void drawMenu() {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextSize(2);
        tft.setCursor(10, 10);
        tft.println("--- MENU ---");

        int interDisp = (int)(interpolationAmount * 10.0f); // 1 to 10
        String items[] = {
            "Return to Radar",
            "Sensitivity: " + String(sensitivity),
            "Loc Averaging: " + String(locationAveraging),
            "Smoothing: " + String(interDisp),
            "Reset Tracking"
        };

        for (int i = 0; i < 5; i++) {
            tft.setCursor(30, 50 + i * 30);
            if (i == menuSelection) {
                tft.setTextColor(TFT_BLACK, TFT_WHITE);
            } else {
                tft.setTextColor(TFT_WHITE, TFT_BLACK);
            }
            tft.println(items[i]);
        }
    }

    void executeMenuAction() {
        switch (menuSelection) {
            case 0:
                state = STATE_RADAR_VIEW;
                tft.fillScreen(TFT_BLACK);
                drawRadarBackground();
                break;
            case 1:
                sensitivity = (sensitivity % 10) + 1;
                drawMenu();
                break;
            case 2:
                locationAveraging = (locationAveraging % 10) + 1;
                drawMenu();
                break;
            case 3:
                interpolationAmount += 0.1f;
                if (interpolationAmount > 1.05f) interpolationAmount = 0.1f;
                drawMenu();
                break;
            case 4:
                actionRequested = 1; // Signal main loop to reset tracking
                state = STATE_RADAR_VIEW;
                tft.fillScreen(TFT_BLACK);
                drawRadarBackground();
                break;
        }
    }
};

#endif
