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
        actionRequested = 0;
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
            if (menuSelection < 0) menuSelection = 3;
            if (menuSelection > 3) menuSelection = 0;
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

    void updateRadarView(RadarTarget targets[3], bool anchorValid, int16_t anchorX, int16_t anchorY) {
        if (state != STATE_RADAR_VIEW) return;

        // Clear previous targets by redrawing background
        tft.fillScreen(TFT_BLACK);
        drawRadarBackground();

        // Draw anchor
        if (anchorValid) {
            tft.setTextColor(TFT_GREEN);
            tft.setCursor(5, 5);
            tft.printf("Anchor: (%d, %d)", anchorX, anchorY);
        } else {
            tft.setTextColor(TFT_RED);
            tft.setCursor(5, 5);
            tft.printf("No Anchor");
        }

        // Draw targets
        for (int i = 0; i < 3; i++) {
            if (targets[i].active) {
                // Ignore targets with speed below sensitivity threshold if not anchor?
                // We'll leave filtering to the main loop or just display all active ones.
                int16_t absSpeed = abs(targets[i].speed);
                if (absSpeed < sensitivity && sensitivity > 1) {
                    continue; // Skip rendering noise if below sensitivity threshold
                }

                // Map mm to screen coordinates (assuming max range 5000mm)
                // Radar is at bottom center (120, 240)
                int screenX = 120 + (targets[i].x * 120 / 5000);
                int screenY = 240 - (targets[i].y * 240 / 5000);

                if (screenX >= 0 && screenX < 240 && screenY >= 0 && screenY < 240) {
                    tft.fillCircle(screenX, screenY, 5, TFT_RED);
                    tft.setTextColor(TFT_YELLOW);
                    tft.setCursor(screenX + 8, screenY - 8);
                    tft.printf("T%d", i + 1);
                }
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

    int actionRequested; // 1=reset

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

        String items[] = {
            "Return to Radar",
            "Sensitivity: " + String(sensitivity),
            "Loc Averaging: " + String(locationAveraging),
            "Reset Tracking"
        };

        for (int i = 0; i < 4; i++) {
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
                actionRequested = 1; // Signal main loop to reset tracking
                state = STATE_RADAR_VIEW;
                tft.fillScreen(TFT_BLACK);
                drawRadarBackground();
                break;
        }
    }
};

#endif
