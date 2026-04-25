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
    STATE_MENU,
    STATE_MENU_EDIT
};

enum ThemeStyle {
    THEME_STANDARD,
    THEME_ALIEN,
    THEME_MINIMAL
};

class UIManager {
public:
    UIManager(TFT_eSPI& display) : tft(display), sprite(&display) {
        state = STATE_RADAR_VIEW;
        menuSelection = 0;

        // Default Settings
        theme = THEME_ALIEN;
        sweepLineEnabled = true;
        trailLength = 5;
        gridEnabled = true;
        sensitivity = 5;
        locationAveraging = 5;
        interpolationAmount = 0.5f;
        actionRequested = 0;

        sweepAngle = 0;
        menuOverlayY = 0;

        // Initialize history
        for (int i=0; i<3; i++) {
            lastTargetActive[i] = false;
            targetCurrentX[i] = 120.0f;
            targetCurrentY[i] = 240.0f;
            lastDrawnX[i] = 120;
            lastDrawnY[i] = 240;

            for (int h=0; h<10; h++) {
                targetHistoryX[i][h] = 120.0f;
                targetHistoryY[i][h] = 240.0f;
            }
        }
    }

    void init() {
        tft.init();
        tft.setRotation(1); // landscape

        sprite.createSprite(240, 240);
        sprite.setSwapBytes(true); // Needed for some TFTs
    }

    void handleEncoder(int dir) {
        if (state == STATE_MENU) {
            menuSelection += dir;
            if (menuSelection < 0) menuSelection = 8;
            if (menuSelection > 8) menuSelection = 0;
        } else if (state == STATE_MENU_EDIT) {
            executeMenuEdit(dir);
        }
    }

    void handleButton() {
        if (state == STATE_RADAR_VIEW) {
            state = STATE_MENU;
            menuSelection = 0;
            menuOverlayY = 0; // Reset animation
        } else if (state == STATE_MENU) {
            if (menuSelection == 8) { // Exit
                state = STATE_RADAR_VIEW;
            } else if (menuSelection == 7) { // Reset Tracking Action
                actionRequested = 1;
                state = STATE_RADAR_VIEW;
            } else {
                state = STATE_MENU_EDIT; // Enter edit mode for this setting
            }
        } else if (state == STATE_MENU_EDIT) {
            state = STATE_MENU; // Save and return to menu scroll
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
        // 1. Update target positions via interpolation
        for (int i = 0; i < 3; i++) {
            if (targetActive[i]) {
                // Save history only if it moved enough (distribute tails evenly)
                float dHx = targetHistoryX[i][0] - targetCurrentX[i];
                float dHy = targetHistoryY[i][0] - targetCurrentY[i];
                if ((dHx*dHx + dHy*dHy) > 10.0f) { // ~3 pixels of movement
                    for (int h = 9; h > 0; h--) {
                        targetHistoryX[i][h] = targetHistoryX[i][h-1];
                        targetHistoryY[i][h] = targetHistoryY[i][h-1];
                    }
                    targetHistoryX[i][0] = targetCurrentX[i];
                    targetHistoryY[i][0] = targetCurrentY[i];
                }

                float diffX = (float)targetGoalX[i] - targetCurrentX[i];
                float diffY = (float)targetGoalY[i] - targetCurrentY[i];

                // If diff is very small, snap it.
                if (abs(diffX) < 0.5f && abs(diffY) < 0.5f) {
                    targetCurrentX[i] = (float)targetGoalX[i];
                    targetCurrentY[i] = (float)targetGoalY[i];
                } else {
                    targetCurrentX[i] += diffX * interpolationAmount;
                    targetCurrentY[i] += diffY * interpolationAmount;
                }
            } else if (lastTargetActive[i]) {
                // Target lost, flush history
                for (int h = 0; h < 10; h++) {
                    targetHistoryX[i][h] = -100;
                    targetHistoryY[i][h] = -100;
                }
            }
            lastTargetActive[i] = targetActive[i];
        }

        // 2. Clear sprite background
        sprite.fillSprite(TFT_BLACK);

        // 3. Draw background elements
        if (theme == THEME_MINIMAL) {
            // Minimal: Just center point and outline
            if (gridEnabled) {
                sprite.drawCircle(120, 240, 180, 0x18E3); // Dark Gray
                sprite.fillCircle(120, 240, 4, 0x18E3);
            }
        } else {
            drawRadarBackground();
        }

        // 4. Draw Sweeping line
        if (sweepLineEnabled && theme != THEME_MINIMAL) {
            sweepAngle = (sweepAngle + 4) % 180;
            float rad = (sweepAngle - 180) * 0.0174533f; // radians
            uint16_t sweepColor = (theme == THEME_ALIEN) ? TFT_GREEN : TFT_DARKGREY;

            // Draw a subtle wedge for trail
            for (int a = 0; a < 15; a += 3) {
                float tr = (sweepAngle - a - 180) * 0.0174533f;
                int tx = 120 + 180 * cos(tr);
                int ty = 240 + 180 * sin(tr);
                // Simple alpha blending fake
                uint16_t trailCol = sprite.alphaBlend((15-a)*17, sweepColor, TFT_BLACK);
                sprite.drawLine(120, 240, tx, ty, trailCol);
            }
        }

        // 5. Draw targets
        for (int i = 0; i < 3; i++) {
            if (targetActive[i]) {
                int cx = (int)targetCurrentX[i];
                int cy = (int)targetCurrentY[i];

                uint16_t color = TFT_RED;
                if (theme == THEME_ALIEN) color = TFT_GREEN;
                else if (theme == THEME_MINIMAL) color = TFT_WHITE;

                // Draw trails
                if (trailLength > 0) {
                    for (int h = 0; h < trailLength; h++) {
                        int hx = (int)targetHistoryX[i][h];
                        int hy = (int)targetHistoryY[i][h];
                        if (hx > 0 && hy > 0) {
                            uint8_t alpha = 255 - ((h * 255) / trailLength);
                            uint16_t tColor = sprite.alphaBlend(alpha, color, TFT_BLACK);
                            int tr = max(1, 4 - (h / 2));
                            sprite.fillCircle(hx, hy, tr, tColor);
                        }
                    }
                }

                // Draw main blip
                if (theme == THEME_ALIEN) {
                    sprite.drawCircle(cx, cy, 6, color);
                    sprite.fillCircle(cx, cy, 3, color);
                } else if (theme == THEME_MINIMAL) {
                    sprite.fillRect(cx - 3, cy - 3, 7, 7, color);
                } else {
                    sprite.fillCircle(cx, cy, 5, color);
                }

                // Text
                if (theme != THEME_ALIEN) {
                    sprite.setTextColor(TFT_YELLOW, TFT_BLACK);
                    sprite.setCursor(cx + 8, cy - 8);
                    sprite.printf("T%d", i + 1);
                }
            }
        }

        // 6. Anchor Status Text
        if (theme != THEME_MINIMAL) {
            if (anchorValid) {
                sprite.setTextColor(TFT_GREEN, TFT_BLACK);
                sprite.setCursor(5, 5);
                sprite.printf("Anchor: (%d, %d)", anchorX, anchorY);
            } else {
                sprite.setTextColor(TFT_RED, TFT_BLACK);
                sprite.setCursor(5, 5);
                sprite.printf("No Anchor");
            }
        }

        // 7. Draw Overlay Menu if active
        if (state == STATE_MENU || state == STATE_MENU_EDIT) {
            drawMenuOverlay();
        }

        // 8. Push to screen
        sprite.pushSprite(0, 0);
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
    TFT_eSprite sprite;
    AppState state;
    int menuSelection;
    int menuOverlayY;

    // Visual Settings
    ThemeStyle theme;
    bool sweepLineEnabled;
    int trailLength;       // 0-10
    bool gridEnabled;

    // Functional Settings
    int sensitivity;       // 1-10
    int locationAveraging; // 1-10
    float interpolationAmount; // 0.1f - 1.0f

    // Rendering vars
    int sweepAngle;

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
    float targetHistoryX[3][10];
    float targetHistoryY[3][10];

    bool lastTargetActive[3];
    int lastDrawnX[3];
    int lastDrawnY[3];

    void drawRadarBackground() {
        uint16_t gridColor = (theme == THEME_ALIEN) ? sprite.color565(0, 50, 0) : TFT_DARKGREY;

        if (gridEnabled) {
            // Draw arcs for distance
            if (theme == THEME_ALIEN) {
                // Dotted arcs simulate
                for (int r=60; r<=180; r+=60) {
                    for (int a=0; a<=180; a+=5) {
                        float rad = (a - 180) * 0.0174533f;
                        sprite.drawPixel(120 + r * cos(rad), 240 + r * sin(rad), gridColor);
                    }
                }
            } else {
                sprite.drawCircle(120, 240, 60, gridColor);
                sprite.drawCircle(120, 240, 120, gridColor);
                sprite.drawCircle(120, 240, 180, gridColor);
            }

            // Draw lines
            sprite.drawLine(120, 240, 120, 60, gridColor);
            sprite.drawLine(120, 240, 60, 240, gridColor);
            sprite.drawLine(120, 240, 180, 240, gridColor);

            if (theme == THEME_ALIEN) {
                // Diagonal lines
                sprite.drawLine(120, 240, 120 - 120*0.707, 240 - 120*0.707, gridColor);
                sprite.drawLine(120, 240, 120 + 120*0.707, 240 - 120*0.707, gridColor);
            }
        }
    }

    void drawMenuOverlay() {
        // Slide in animation for overlay
        if (menuOverlayY < 120) menuOverlayY += 10;

        // Draw a semi-transparent or solid panel on the left/top
        sprite.fillRect(0, 0, 240, menuOverlayY, sprite.alphaBlend(200, TFT_BLACK, TFT_WHITE));
        sprite.drawLine(0, menuOverlayY, 240, menuOverlayY, TFT_DARKGREY);

        if (menuOverlayY < 120) return; // Wait for animation

        sprite.setTextSize(1);

        String themeStr = (theme == THEME_STANDARD) ? "Standard" : (theme == THEME_ALIEN ? "Alien" : "Minimal");
        int interDisp = (int)(interpolationAmount * 10.0f + 0.5f);

        String items[9] = {
            "Theme: " + themeStr,
            "Sweep Line: " + String(sweepLineEnabled ? "ON" : "OFF"),
            "Trails: " + String(trailLength),
            "Grid: " + String(gridEnabled ? "ON" : "OFF"),
            "Loc Avg: " + String(locationAveraging),
            "Sensitivity: " + String(sensitivity),
            "Smoothing: " + String(interDisp),
            "[ Reset Tracking ]",
            "[ Exit Menu ]"
        };

        // We only have space to show ~4-5 items, implement scrolling
        int startIdx = max(0, menuSelection - 2);
        if (startIdx > 3) startIdx = 3;

        for (int i = 0; i < 5; i++) {
            int idx = startIdx + i;
            if (idx > 8) break;

            int yPos = 10 + i * 20;

            if (idx == menuSelection) {
                if (state == STATE_MENU_EDIT) {
                    sprite.fillRect(5, yPos - 2, 230, 18, TFT_DARKGREY); // Editing highlight
                    sprite.setTextColor(TFT_GREEN, TFT_DARKGREY);
                } else {
                    sprite.fillRect(5, yPos - 2, 230, 18, TFT_WHITE); // Selected highlight
                    sprite.setTextColor(TFT_BLACK, TFT_WHITE);
                }
            } else {
                sprite.setTextColor(TFT_WHITE, TFT_BLACK); // Normal
            }

            sprite.setCursor(15, yPos);
            sprite.print(items[idx]);
        }
    }

    void executeMenuEdit(int dir) {
        switch (menuSelection) {
            case 0: { // Theme
                int t = (int)theme + dir;
                if (t > 2) t = 0;
                if (t < 0) t = 2;
                theme = (ThemeStyle)t;
                // Apply presets
                if (theme == THEME_ALIEN) { sweepLineEnabled = true; trailLength = 8; gridEnabled = true; }
                else if (theme == THEME_MINIMAL) { sweepLineEnabled = false; trailLength = 0; gridEnabled = true; }
                else { sweepLineEnabled = true; trailLength = 3; gridEnabled = true; }
                break; }
            case 1: // Sweep Line
                sweepLineEnabled = !sweepLineEnabled;
                break;
            case 2: // Trails
                trailLength += dir;
                if (trailLength < 0) trailLength = 0;
                if (trailLength > 10) trailLength = 10;
                break;
            case 3: // Grid
                gridEnabled = !gridEnabled;
                break;
            case 4: // Loc Avg
                locationAveraging += dir;
                if (locationAveraging < 1) locationAveraging = 1;
                if (locationAveraging > 10) locationAveraging = 10;
                break;
            case 5: // Sensitivity
                sensitivity += dir;
                if (sensitivity < 1) sensitivity = 1;
                if (sensitivity > 10) sensitivity = 10;
                break;
            case 6: // Smoothing
                interpolationAmount += (dir * 0.1f);
                if (interpolationAmount < 0.1f) interpolationAmount = 0.1f;
                if (interpolationAmount > 1.05f) interpolationAmount = 1.0f;
                break;
        }
    }
};

#endif
