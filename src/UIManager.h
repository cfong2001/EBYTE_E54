#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <RotaryEncoder.h>
#include <OneButton.h>
#include <Preferences.h>
#include "E54_Radar.h"
#include "ZoneManager.h"

enum AppState {
    STATE_BOOT,
    STATE_RADAR_VIEW,
    STATE_MENU,
    STATE_MENU_EDIT
};

enum MenuPage {
    PAGE_MAIN,
    PAGE_VISUALS,
    PAGE_ZONES,
    PAGE_DATA
};

enum ThemeStyle {
    THEME_STANDARD,
    THEME_ALIEN,
    THEME_MINIMAL
};

enum TelemetryMode {
    TELEMETRY_OFF,
    TELEMETRY_DIST_ANG,
    TELEMETRY_VELOCITY,
    TELEMETRY_RAW,
    TELEMETRY_ALL
};

enum TargetIcon {
    ICON_CIRCLE,
    ICON_SQUARE,
    ICON_TRIANGLE,
    ICON_SMART
};

class UIManager {
public:
    bool serialDebugEnabled;
public:
    ZoneManager zoneManager;
    Preferences preferences;

    UIManager(TFT_eSPI& display) : tft(display), sprite(&display) {
        state = STATE_BOOT;
        activePage = PAGE_MAIN;
        menuSelection = 0;

        theme = THEME_ALIEN;
        targetIcon = ICON_SMART;
        sweepLineEnabled = true;
        trailLength = 5;
        gridEnabled = true;
        startupAnimEnabled = true;
        simulatedSweep = false;

        telemetryMode = TELEMETRY_OFF;
        sensitivity = 5;
        locationAveraging = 5;
        interpolationAmount = 0.5f;
        serialDebugEnabled = false;
        actionRequested = 0;

        sweepAngle = 0;
        menuOverlayY = 0;
        maxMenuSelection = 0;
        bootStartTime = 0;

        for (int i=0; i<3; i++) {
            lastTargetActive[i] = false;
            targetCurrentX[i] = 120.0f;
            targetCurrentY[i] = 240.0f;
            lastDrawnX[i] = 120;
            lastDrawnY[i] = 240;

            rawTargetX[i] = 0;
            rawTargetY[i] = 0;
            rawTargetSpeed[i] = 0;
            simAlpha[i] = 0.0f;
            smoothVecX[i] = 0.0f;
            smoothVecY[i] = 0.0f;
            smoothSpeed[i] = 0.0f;

            for (int h=0; h<10; h++) {
                targetHistoryX[i][h] = 120.0f;
                targetHistoryY[i][h] = 240.0f;
            }
        }
    }

    void loadSettings() {
        preferences.begin("radar_ui", false);
        theme = (ThemeStyle)preferences.getInt("theme", THEME_ALIEN);
        targetIcon = (TargetIcon)preferences.getInt("icon", ICON_SMART);
        sweepLineEnabled = preferences.getBool("sweep", true);
        trailLength = preferences.getInt("trails", 5);
        gridEnabled = preferences.getBool("grid", true);
        startupAnimEnabled = preferences.getBool("startup", true);
        simulatedSweep = preferences.getBool("simSwp", false);

        telemetryMode = (TelemetryMode)preferences.getInt("tData", TELEMETRY_OFF);
        sensitivity = preferences.getInt("sens", 5);
        locationAveraging = preferences.getInt("locAvg", 5);
        interpolationAmount = (float)preferences.getInt("interp", 5) / 10.0f;
        serialDebugEnabled = preferences.getBool("serDebug", false);
        preferences.end();
    }

    void saveSettings() {
        preferences.begin("radar_ui", false);
        preferences.putInt("theme", theme);
        preferences.putInt("icon", targetIcon);
        preferences.putBool("sweep", sweepLineEnabled);
        preferences.putInt("trails", trailLength);
        preferences.putBool("grid", gridEnabled);
        preferences.putBool("startup", startupAnimEnabled);
        preferences.putBool("simSwp", simulatedSweep);

        preferences.putInt("tData", telemetryMode);
        preferences.putInt("sens", sensitivity);
        preferences.putInt("locAvg", locationAveraging);
        int interDisp = (int)(interpolationAmount * 10.0f + 0.5f);
        preferences.putInt("interp", interDisp);
        preferences.putBool("serDebug", serialDebugEnabled);
        preferences.end();
    }

    void init() {
        zoneManager.loadSettings();
        loadSettings();

        tft.init();
        tft.setRotation(1);
        sprite.createSprite(240, 240);
        sprite.setSwapBytes(true);

        if (startupAnimEnabled) {
            state = STATE_BOOT;
            bootStartTime = millis();
        } else {
            state = STATE_RADAR_VIEW;
        }
    }

    void handleEncoder(int dir) {
        if (state == STATE_MENU) {
            menuSelection += dir;
            if (menuSelection > maxMenuSelection) menuSelection = 0;
            if (menuSelection < 0) menuSelection = maxMenuSelection;
        } else if (state == STATE_MENU_EDIT) {
            executeMenuEdit(dir);
        }
    }

    void handleButton() {
        if (state == STATE_RADAR_VIEW) {
            state = STATE_MENU;
            activePage = PAGE_MAIN;
            menuSelection = 0;
            menuOverlayY = 0;
        } else if (state == STATE_MENU) {
            handleMenuClick();
        } else if (state == STATE_MENU_EDIT) {
            saveSettings();
            state = STATE_MENU;
        }
    }



    void setTargetMotion(int index, float vx, float vy, float ax, float ay) {
        if (index >= 0 && index < 3) {
            // Convert mm/s to screen pixels
            targetVelX[index] = vx * 120 / 5000;
            targetVelY[index] = -vy * 240 / 5000; // Y is inverted on screen
            targetAccX[index] = ax * 120 / 5000;
            targetAccY[index] = -ay * 240 / 5000;
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

                    rawTargetX[i] = targets[i].x;
                    rawTargetY[i] = targets[i].y;
                    rawTargetSpeed[i] = targets[i].speed;

                    if (targetGoalX[i] < 0 || targetGoalX[i] >= 240 || targetGoalY[i] < 0 || targetGoalY[i] >= 240) {
                        targetActive[i] = false;
                    }
                }
            }

            if (targetActive[i] && !lastTargetActive[i]) {
                targetCurrentX[i] = (float)targetGoalX[i];
                targetCurrentY[i] = (float)targetGoalY[i];
            }
        }
    }

    void renderLoop() {
        if (state == STATE_BOOT) {
            drawBootScreen();
            return;
        }

        // Advance logic
        for (int i = 0; i < 3; i++) {
            if (targetActive[i]) {

                float dHx = targetHistoryX[i][0] - targetCurrentX[i];
                float dHy = targetHistoryY[i][0] - targetCurrentY[i];
                if ((dHx*dHx + dHy*dHy) > 10.0f) {
                    for (int h = 9; h > 0; h--) {
                        targetHistoryX[i][h] = targetHistoryX[i][h-1];
                        targetHistoryY[i][h] = targetHistoryY[i][h-1];
                    }
                    targetHistoryX[i][0] = targetCurrentX[i];
                    targetHistoryY[i][0] = targetCurrentY[i];
                }


                // To utilize the advanced prediction algorithm (alpha-beta filter velocities)
                // we calculate a predicted goal based on targetVelX/Y over a time step,
                // but since updateRadarData sets the "absolute" targetGoalX/Y from the motion
                // compensated coordinates, we simply smoothly interpolate towards the goal, potentially
                // using the velocity vector to curve or predict the path.


                float diffX = (float)targetGoalX[i] - targetCurrentX[i];
                float diffY = (float)targetGoalY[i] - targetCurrentY[i];

                if (fabsf(diffX) < 0.5f && fabsf(diffY) < 0.5f) {
                    targetCurrentX[i] = (float)targetGoalX[i];
                    targetCurrentY[i] = (float)targetGoalY[i];
                } else {
                    // Combine standard linear interpolation with curved predictive velocity/acceleration feed-forward
                    // Approximates the next position step using velocity + acceleration curve
                    float t = 0.03f; // DT ~30ms render loop
                    float t_sq_half = (t * t) * 0.5f;

                    float curveForwardX = (targetVelX[i] * t) + (targetAccX[i] * t_sq_half);
                    float curveForwardY = (targetVelY[i] * t) + (targetAccY[i] * t_sq_half);

                    targetCurrentX[i] += (diffX * interpolationAmount) + curveForwardX;
                    targetCurrentY[i] += (diffY * interpolationAmount) + curveForwardY;
                }

                // Simulated Sweep Capture Logic
                if (simulatedSweep) {
                    float targetRad = atan2f(rawTargetX[i], rawTargetY[i]);
                    int targetDeg = (int)(targetRad * 180.0f / PI);

                    // The visual sweepAngle goes from 0 to 180. We map it to -90 to 90.
                    int visualAngle = sweepAngle - 90;

                    // If visual angle passes the target, snap alpha to 1.0
                    // We check if it's within a 5 degree wedge.
                    if (abs(visualAngle - targetDeg) < 5) {
                        simAlpha[i] = 1.0f;
                    }
                    // Decay alpha slowly
                    simAlpha[i] -= 0.03f;
                    if (simAlpha[i] < 0.0f) simAlpha[i] = 0.0f;
                } else {
                    simAlpha[i] = 1.0f; // Always visible
                }

            } else if (lastTargetActive[i]) {
                for (int h = 0; h < 10; h++) {
                    targetHistoryX[i][h] = -100;
                    targetHistoryY[i][h] = -100;
                }
                simAlpha[i] = 0.0f;
                smoothVecX[i] = 0.0f;
                smoothVecY[i] = 0.0f;
                smoothSpeed[i] = 0.0f;
            }
            lastTargetActive[i] = targetActive[i];
        }

        sprite.fillSprite(TFT_BLACK);

        if (theme == THEME_MINIMAL) {
            if (gridEnabled) {
                sprite.drawCircle(120, 240, 180, 0x18E3);
                sprite.fillCircle(120, 240, 4, 0x18E3);
            }
        } else {
            drawRadarBackground();
        }

        drawZones();

        if (sweepLineEnabled && theme != THEME_MINIMAL) {
            sweepAngle = (sweepAngle + 4) % 180;
            uint16_t sweepColor = (theme == THEME_ALIEN) ? TFT_GREEN : TFT_DARKGREY;
            for (int a = 0; a < 30; a += 2) {
                float tr = (sweepAngle - a - 180) * 0.0174533f;
                int tx = 120 + 180 * cosf(tr);
                int ty = 240 + 180 * sinf(tr);
                uint8_t alpha = 255 - ((a * 255) / 30);
                uint16_t trailCol = sprite.alphaBlend(alpha, sweepColor, TFT_BLACK);
                sprite.drawLine(120, 240, tx, ty, trailCol);
            }
        }

        for (int i = 0; i < 3; i++) {
            if (targetActive[i] && simAlpha[i] > 0.01f) {
                int cx = (int)targetCurrentX[i];
                int cy = (int)targetCurrentY[i];

                uint16_t baseColor;
                if (theme == THEME_MINIMAL) {
                    baseColor = TFT_WHITE;
                } else if (theme == THEME_ALIEN) {
                    if (i == 0) baseColor = sprite.color565(0, 255, 0); // Green
                    else if (i == 1) baseColor = sprite.color565(0, 255, 255); // Cyan
                    else baseColor = sprite.color565(255, 0, 255); // Magenta
                } else {
                    if (i == 0) baseColor = sprite.color565(255, 100, 0); // Orange
                    else if (i == 1) baseColor = sprite.color565(0, 150, 255); // Blue
                    else baseColor = sprite.color565(200, 0, 255); // Purple
                }

                // Blend with black based on sweep simulation alpha
                uint8_t currentAlpha = (uint8_t)(simAlpha[i] * 255.0f);
                uint16_t color = sprite.alphaBlend(currentAlpha, baseColor, TFT_BLACK);

                if (trailLength > 0) {
                    for (int h = 0; h < trailLength; h++) {
                        int hx = (int)targetHistoryX[i][h];
                        int hy = (int)targetHistoryY[i][h];
                        if (hx > 0 && hy > 0) {
                            uint8_t t_alpha = (currentAlpha * (trailLength - h)) / trailLength;
                            uint16_t tColor = sprite.alphaBlend(t_alpha, baseColor, TFT_BLACK);
                            int tr = max(1, 4 - (h / 2));
                            sprite.fillCircle(hx, hy, tr, tColor);
                        }
                    }
                }

                if (zoneManager.isWarning(i)) {
                    if ((millis() / 200) % 2 == 0) {
                        uint16_t wCol = sprite.alphaBlend(currentAlpha, TFT_YELLOW, TFT_BLACK);
                        sprite.drawCircle(cx, cy, 8, wCol);
                    } else {
                        uint16_t wCol = sprite.alphaBlend(currentAlpha, TFT_RED, TFT_BLACK);
                        sprite.drawCircle(cx, cy, 8, wCol);
                    }
                }

                // Reticles
                if (targetIcon == ICON_CIRCLE) {
                    sprite.fillCircle(cx, cy, 4, color);
                }
                else if (targetIcon == ICON_SQUARE) {
                    sprite.fillRect(cx - 3, cy - 3, 7, 7, color);
                }
                else if (targetIcon == ICON_TRIANGLE) {
                    sprite.fillTriangle(cx, cy - 5, cx - 4, cy + 3, cx + 4, cy + 3, color);
                }
                else if (targetIcon == ICON_SMART) {
                    sprite.drawCircle(cx, cy, 3, color);

                    int absSpd = abs(rawTargetSpeed[i]);
                    float rawDx = targetCurrentX[i] - targetHistoryX[i][2];
                    float rawDy = targetCurrentY[i] - targetHistoryY[i][2];

                    if (absSpd > 10 && (fabsf(rawDx) > 0.5f || fabsf(rawDy) > 0.5f)) {
                        float len = sqrtf(rawDx*rawDx + rawDy*rawDy);
                        float nx = rawDx / len;
                        float ny = rawDy / len;
                        smoothVecX[i] = (smoothVecX[i] * 0.7f) + (nx * 0.3f);
                        smoothVecY[i] = (smoothVecY[i] * 0.7f) + (ny * 0.3f);
                        smoothSpeed[i] = (smoothSpeed[i] * 0.8f) + ((float)absSpd * 0.2f);
                    } else {
                        smoothSpeed[i] *= 0.8f;
                    }

                    if (smoothSpeed[i] > 5.0f) {
                        float stickLen = 5.0f + (smoothSpeed[i] / 10.0f);
                        if (stickLen > 25.0f) stickLen = 25.0f;
                        float sLen = sqrtf(smoothVecX[i]*smoothVecX[i] + smoothVecY[i]*smoothVecY[i]);
                        if (sLen > 0.01f) {
                            float nSvx = smoothVecX[i] / sLen;
                            float nSvy = smoothVecY[i] / sLen;
                            int ex = cx + (int)(nSvx * stickLen);
                            int ey = cy + (int)(nSvy * stickLen);
                            sprite.drawLine(cx, cy, ex, ey, color);

                            float arrowAngle = atan2f(nSvy, nSvx);
                            int ax1 = ex - (int)(4 * cosf(arrowAngle - 0.5f));
                            int ay1 = ey - (int)(4 * sinf(arrowAngle - 0.5f));
                            int ax2 = ex - (int)(4 * cosf(arrowAngle + 0.5f));
                            int ay2 = ey - (int)(4 * sinf(arrowAngle + 0.5f));
                            sprite.drawLine(ex, ey, ax1, ay1, color);
                            sprite.drawLine(ex, ey, ax2, ay2, color);
                        }
                    } else {
                        sprite.fillCircle(cx, cy, 2, color);
                    }
                }

                // Telemetry Text
                if (theme != THEME_ALIEN && telemetryMode != TELEMETRY_OFF) {
                    sprite.setTextColor(color, TFT_BLACK);

                    float dist_m = sqrtf((long)rawTargetX[i]*rawTargetX[i] + (long)rawTargetY[i]*rawTargetY[i]) / 1000.0f;
                    int angle = (int)(atan2f((float)rawTargetX[i], (float)rawTargetY[i]) * 180.0f / PI);
                    float speed_ms = (float)rawTargetSpeed[i] / 10.0f; // Assuming 10s of cm/s or similar, pseudo-calc

                    sprite.setCursor(cx + 8, cy - 12);

                    if (telemetryMode == TELEMETRY_DIST_ANG) {
                        sprite.printf("%.1fm %d", dist_m, angle);
                    } else if (telemetryMode == TELEMETRY_VELOCITY) {
                        sprite.printf("%.1fm/s", speed_ms);
                    } else if (telemetryMode == TELEMETRY_RAW) {
                        sprite.printf("%d,%d", rawTargetX[i], rawTargetY[i]);
                    } else if (telemetryMode == TELEMETRY_ALL) {
                        sprite.printf("T%d %.1fm %d", i+1, dist_m, angle);
                        sprite.setCursor(cx + 8, cy - 2);
                        sprite.printf("%.1fm/s", speed_ms);
                    }
                } else if (theme != THEME_ALIEN && telemetryMode == TELEMETRY_OFF) {
                    sprite.setTextColor(color, TFT_BLACK);
                    sprite.setCursor(cx + 8, cy - 8);
                    sprite.printf("T%d", i + 1);
                }
            }
        }

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

        if (state == STATE_MENU || state == STATE_MENU_EDIT) {
            drawMenuOverlay();
        }

        sprite.pushSprite(0, 0);
    }

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
    MenuPage activePage;
    int menuSelection;
    int menuOverlayY;
    int maxMenuSelection;
    unsigned long bootStartTime;

    ThemeStyle theme;
    TargetIcon targetIcon;
    bool sweepLineEnabled;
    int trailLength;
    bool gridEnabled;
    bool startupAnimEnabled;
    bool simulatedSweep;

    TelemetryMode telemetryMode;
    int sensitivity;
    int locationAveraging;
    float interpolationAmount;

    int sweepAngle;
    int actionRequested;

    bool anchorValid;
    int16_t anchorX;
    int16_t anchorY;

    bool targetActive[3];
    int targetGoalX[3];
    int targetGoalY[3];

    int16_t rawTargetX[3];
    int16_t rawTargetY[3];
    int16_t rawTargetSpeed[3];

    float targetCurrentX[3];
    float targetCurrentY[3];
    float targetVelX[3];
    float targetVelY[3];
    float targetAccX[3];
    float targetAccY[3];

    float targetHistoryX[3][10];
    float targetHistoryY[3][10];
    float simAlpha[3];
    float smoothVecX[3];
    float smoothVecY[3];
    float smoothSpeed[3];

    bool lastTargetActive[3];
    int lastDrawnX[3];
    int lastDrawnY[3];

    void drawBootScreen() {
        sprite.fillSprite(TFT_BLACK);
        unsigned long elapsed = millis() - bootStartTime;

        int maxR = (elapsed * 180) / 1000;
        if (maxR > 180) maxR = 180;

        uint16_t gridColor = (theme == THEME_ALIEN) ? sprite.color565(0, 100, 0) : sprite.color565(100, 100, 100);

        for (int r = 60; r <= 180; r += 60) {
            if (maxR >= r) {
                int sweepDeg = ((maxR - r) * 180) / 30;
                if (sweepDeg > 360) sweepDeg = 360;
                for (int a = -180; a < -180 + sweepDeg; a += 5) {
                    float rad = a * 0.0174533f;
                    sprite.drawPixel(120 + r * cosf(rad), 240 + r * sinf(rad), gridColor);
                }
            }
        }

        if (maxR > 0) {
            sprite.drawLine(120, 240, 120, 240 - maxR, gridColor);
            sprite.drawLine(120, 240, 120 - maxR, 240, gridColor);
            sprite.drawLine(120, 240, 120 + maxR, 240, gridColor);
        }

        sprite.setTextColor(gridColor, TFT_BLACK);
        sprite.setTextSize(1);
        if (elapsed < 300) sprite.setCursor(100, 120), sprite.print("INIT");
        else if (elapsed < 600) sprite.setCursor(90, 120), sprite.print("CALIBRATING");
        else if (elapsed < 1000) sprite.setCursor(95, 120), sprite.print("SCANNING...");

        sprite.pushSprite(0, 0);

        if (elapsed > 1200) {
            state = STATE_RADAR_VIEW;
        }
    }

    void drawRadialWedge(int minDist, int maxDist, int minAngle, int maxAngle, uint16_t color) {
        int maxR = (maxDist * 180) / 6000;
        int minR = (minDist * 180) / 6000;
        if (maxR > 180) maxR = 180;
        if (minR > 180) minR = 180;
        for (int a = minAngle; a <= maxAngle; a++) {
            float rad = (a - 90) * 0.0174533f;
            float cosA = cosf(rad);
            float sinA = sinf(rad);
            sprite.drawLine(120 + minR*cosA, 240 + minR*sinA, 120 + maxR*cosA, 240 + maxR*sinA, color);
        }
    }

    void drawZones() {
        if (zoneManager.getDeadPreset() != ZONE_OFF) {
            RadialZone z = zoneManager.getActiveDeadZone();
            drawRadialWedge(z.minDist, z.maxDist, z.minAngle, z.maxAngle, sprite.color565(30, 0, 0));
        }
        if (zoneManager.getWarnPreset() != ZONE_OFF) {
            RadialZone z = zoneManager.getActiveWarnZone();
            float danger = zoneManager.getDangerLevel();
            uint16_t baseColor = sprite.color565(50, 50, 0);
            uint16_t activeColor = sprite.color565(255, 100, 0);
            uint8_t alpha = (uint8_t)(danger * 255.0f);
            uint16_t wedgeColor = sprite.alphaBlend(alpha, activeColor, baseColor);
            drawRadialWedge(z.minDist, z.maxDist, z.minAngle, z.maxAngle, wedgeColor);
        }
    }

    void drawRadarBackground() {
        uint16_t gridColor = (theme == THEME_ALIEN) ? sprite.color565(0, 50, 0) : TFT_DARKGREY;
        if (gridEnabled) {
            if (theme == THEME_ALIEN) {
                for (int r=60; r<=180; r+=60) {
                    for (int a=0; a<=180; a+=5) {
                        float rad = (a - 180) * 0.0174533f;
                        sprite.drawPixel(120 + r * cosf(rad), 240 + r * sinf(rad), gridColor);
                    }
                }
            } else {
                sprite.drawCircle(120, 240, 60, gridColor);
                sprite.drawCircle(120, 240, 120, gridColor);
                sprite.drawCircle(120, 240, 180, gridColor);
            }
            sprite.drawLine(120, 240, 120, 60, gridColor);
            sprite.drawLine(120, 240, 60, 240, gridColor);
            sprite.drawLine(120, 240, 180, 240, gridColor);
            if (theme == THEME_ALIEN) {
                sprite.drawLine(120, 240, 120 - 120*0.707, 240 - 120*0.707, gridColor);
                sprite.drawLine(120, 240, 120 + 120*0.707, 240 - 120*0.707, gridColor);
            }
        }
    }

    void drawMenuOverlay() {
        if (menuOverlayY < 120) menuOverlayY += 10;

        sprite.fillRect(0, 0, 240, menuOverlayY, sprite.alphaBlend(200, TFT_BLACK, TFT_WHITE));
        sprite.drawLine(0, menuOverlayY, 240, menuOverlayY, TFT_DARKGREY);
        if (menuOverlayY < 120) return;

        sprite.setTextSize(1);
        String items[24];
        int numItems = 0;

        if (activePage == PAGE_MAIN) {
            sprite.setTextColor(TFT_GREEN, TFT_BLACK);
            sprite.setCursor(15, 5); sprite.print("=== MAIN MENU ===");

            items[numItems++] = "> VISUAL SETTINGS";
            items[numItems++] = "> ZONE CONFIG";
            items[numItems++] = "> TARGET DATA & SENS";
            items[numItems++] = "[ Exit Menu ]";
        }
        else if (activePage == PAGE_VISUALS) {
            sprite.setTextColor(TFT_GREEN, TFT_BLACK);
            sprite.setCursor(15, 5); sprite.print("--- VISUAL SETTINGS ---");

            String themeStr = (theme == THEME_STANDARD) ? "Standard" : (theme == THEME_ALIEN ? "Alien" : "Minimal");
            String iconStr = (targetIcon == ICON_CIRCLE) ? "CIRCLE" :
                             (targetIcon == ICON_SQUARE) ? "SQUARE" :
                             (targetIcon == ICON_TRIANGLE) ? "TRIANGLE" : "SMART";
            items[numItems++] = "< Back";
            items[numItems++] = "Theme: " + themeStr;
            items[numItems++] = "Icon: " + iconStr;
            items[numItems++] = "Sweep Line: " + String(sweepLineEnabled ? "ON" : "OFF");
            items[numItems++] = "Sweep Mode: " + String(simulatedSweep ? "SIMULATED" : "VISUAL");
            items[numItems++] = "Trails: " + String(trailLength);
            items[numItems++] = "Grid: " + String(gridEnabled ? "ON" : "OFF");
            items[numItems++] = "Boot Anim: " + String(startupAnimEnabled ? "ON" : "OFF");
        }
        else if (activePage == PAGE_ZONES) {
            sprite.setTextColor(TFT_GREEN, TFT_BLACK);
            sprite.setCursor(15, 5); sprite.print("--- ZONE CONFIG ---");

            String warnStr = (zoneManager.getWarnPreset() == ZONE_OFF) ? "OFF" :
                             (zoneManager.getWarnPreset() == ZONE_CLOSE) ? "CLOSE" :
                             (zoneManager.getWarnPreset() == ZONE_MEDIUM) ? "MED" :
                             (zoneManager.getWarnPreset() == ZONE_FAR) ? "FAR" : "CUSTOM";

            String deadStr = (zoneManager.getDeadPreset() == ZONE_OFF) ? "OFF" :
                             (zoneManager.getDeadPreset() == ZONE_CLOSE) ? "CLOSE" :
                             (zoneManager.getDeadPreset() == ZONE_MEDIUM) ? "MED" :
                             (zoneManager.getDeadPreset() == ZONE_FAR) ? "FAR" : "CUSTOM";

            items[numItems++] = "< Back";
            items[numItems++] = "Warn Zone: " + warnStr;
            if (zoneManager.getWarnPreset() == ZONE_CUSTOM) {
                items[numItems++] = " W-MinD: " + String(zoneManager.getWarnCustom().minDist);
                items[numItems++] = " W-MaxD: " + String(zoneManager.getWarnCustom().maxDist);
                items[numItems++] = " W-MinA: " + String(zoneManager.getWarnCustom().minAngle);
                items[numItems++] = " W-MaxA: " + String(zoneManager.getWarnCustom().maxAngle);
            }
            if (zoneManager.getWarnPreset() != ZONE_OFF) {
                items[numItems++] = "Warn Fuzz: " + String(zoneManager.getFuzzingThreshold()) + "%";
                items[numItems++] = "Warn Time: " + String(zoneManager.getHistoryWindow() * 100) + "ms";
            }

            items[numItems++] = "Dead Zone: " + deadStr;
            if (zoneManager.getDeadPreset() == ZONE_CUSTOM) {
                items[numItems++] = " D-MinD: " + String(zoneManager.getDeadCustom().minDist);
                items[numItems++] = " D-MaxD: " + String(zoneManager.getDeadCustom().maxDist);
                items[numItems++] = " D-MinA: " + String(zoneManager.getDeadCustom().minAngle);
                items[numItems++] = " D-MaxA: " + String(zoneManager.getDeadCustom().maxAngle);
            }
        }
        else if (activePage == PAGE_DATA) {
            sprite.setTextColor(TFT_GREEN, TFT_BLACK);
            sprite.setCursor(15, 5); sprite.print("--- TARGET DATA ---");

            String tDataStr = (telemetryMode == TELEMETRY_OFF) ? "OFF" :
                              (telemetryMode == TELEMETRY_DIST_ANG) ? "DIST/ANG" :
                              (telemetryMode == TELEMETRY_VELOCITY) ? "SPEED" :
                              (telemetryMode == TELEMETRY_RAW) ? "RAW X/Y" : "ALL";
            int interDisp = (int)(interpolationAmount * 10.0f + 0.5f);

            items[numItems++] = "< Back";
            items[numItems++] = "Telemetry: " + tDataStr;
            items[numItems++] = "Sensitivity: " + String(sensitivity);
            items[numItems++] = "Loc Avg: " + String(locationAveraging);
            items[numItems++] = "Smoothing: " + String(interDisp);
            items[numItems++] = "Ser Debug: " + String(serialDebugEnabled ? "ON" : "OFF");
            items[numItems++] = "[ Reset Tracking ]";
        }

        maxMenuSelection = numItems - 1;

        int startIdx = max(0, menuSelection - 2);
        if (startIdx > numItems - 4) startIdx = max(0, numItems - 4);

        for (int i = 0; i < 4; i++) {
            int idx = startIdx + i;
            if (idx >= numItems) break;

            int yPos = 25 + i * 20;

            if (idx == menuSelection) {
                if (state == STATE_MENU_EDIT) {
                    sprite.fillRect(5, yPos - 2, 230, 18, TFT_DARKGREY);
                    sprite.setTextColor(TFT_GREEN, TFT_DARKGREY);
                } else {
                    sprite.fillRect(5, yPos - 2, 230, 18, TFT_WHITE);
                    sprite.setTextColor(TFT_BLACK, TFT_WHITE);
                }
            } else {
                sprite.setTextColor(TFT_WHITE, TFT_BLACK);
            }

            sprite.setCursor(15, yPos);
            sprite.print(items[idx]);
        }
    }

    void handleMenuClick() {
        if (activePage == PAGE_MAIN) {
            if (menuSelection == 0) { activePage = PAGE_VISUALS; menuSelection = 0; }
            else if (menuSelection == 1) { activePage = PAGE_ZONES; menuSelection = 0; }
            else if (menuSelection == 2) { activePage = PAGE_DATA; menuSelection = 0; }
            else if (menuSelection == 3) { state = STATE_RADAR_VIEW; }
        }
        else if (activePage == PAGE_VISUALS) {
            if (menuSelection == 0) { activePage = PAGE_MAIN; menuSelection = 0; }
            else { state = STATE_MENU_EDIT; }
        }
        else if (activePage == PAGE_ZONES) {
            if (menuSelection == 0) { activePage = PAGE_MAIN; menuSelection = 0; }
            else { state = STATE_MENU_EDIT; }
        }
        else if (activePage == PAGE_DATA) {
            if (menuSelection == 0) { activePage = PAGE_MAIN; menuSelection = 0; }
            else if (menuSelection == maxMenuSelection) { actionRequested = 1; state = STATE_RADAR_VIEW; }
            else { state = STATE_MENU_EDIT; }
        }
    }

    void executeMenuEdit(int dir) {
        int idx = 1; // 0 is always < Back

        if (activePage == PAGE_VISUALS) {
            if (idx++ == menuSelection) {
                int t = (int)theme + dir;
                if (t > 2) t = 0; if (t < 0) t = 2;
                theme = (ThemeStyle)t;
                if (theme == THEME_ALIEN) { sweepLineEnabled = true; trailLength = 8; gridEnabled = true; }
                else if (theme == THEME_MINIMAL) { sweepLineEnabled = false; trailLength = 0; gridEnabled = true; }
                else { sweepLineEnabled = true; trailLength = 3; gridEnabled = true; }
                return;
            }
            if (idx++ == menuSelection) {
                int ic = (int)targetIcon + dir;
                if (ic > 3) ic = 0; if (ic < 0) ic = 3;
                targetIcon = (TargetIcon)ic;
                return;
            }
            if (idx++ == menuSelection) { sweepLineEnabled = !sweepLineEnabled; return; }
            if (idx++ == menuSelection) { simulatedSweep = !simulatedSweep; return; }
            if (idx++ == menuSelection) { trailLength += dir; if (trailLength < 0) trailLength = 0; if (trailLength > 10) trailLength = 10; return; }
            if (idx++ == menuSelection) { gridEnabled = !gridEnabled; return; }
            if (idx++ == menuSelection) { startupAnimEnabled = !startupAnimEnabled; return; }
        }
        else if (activePage == PAGE_ZONES) {
            if (idx++ == menuSelection) {
                int p = (int)zoneManager.getWarnPreset() + dir;
                if (p > 4) p = 0; if (p < 0) p = 4;
                zoneManager.setWarnPreset((ZonePreset)p);
                return;
            }
            if (zoneManager.getWarnPreset() == ZONE_CUSTOM) {
                RadialZone z = zoneManager.getWarnCustom();
                if (idx++ == menuSelection) { z.minDist += dir * 100; if(z.minDist < 0) z.minDist=0; zoneManager.setWarnCustom(z); return; }
                if (idx++ == menuSelection) { z.maxDist += dir * 100; if(z.maxDist < z.minDist) z.maxDist=z.minDist; zoneManager.setWarnCustom(z); return; }
                if (idx++ == menuSelection) { z.minAngle += dir * 5; if(z.minAngle < -90) z.minAngle=-90; zoneManager.setWarnCustom(z); return; }
                if (idx++ == menuSelection) { z.maxAngle += dir * 5; if(z.maxAngle > 90) z.maxAngle=90; zoneManager.setWarnCustom(z); return; }
            }
            if (zoneManager.getWarnPreset() != ZONE_OFF) {
                if (idx++ == menuSelection) {
                    int f = zoneManager.getFuzzingThreshold() + dir * 5;
                    if (f < 0) f = 0; if (f > 100) f = 100;
                    zoneManager.setFuzzingThreshold(f);
                    return;
                }
                if (idx++ == menuSelection) {
                    zoneManager.setHistoryWindow(zoneManager.getHistoryWindow() + dir);
                    return;
                }
            }
            if (idx++ == menuSelection) {
                int p = (int)zoneManager.getDeadPreset() + dir;
                if (p > 4) p = 0; if (p < 0) p = 4;
                zoneManager.setDeadPreset((ZonePreset)p);
                return;
            }
            if (zoneManager.getDeadPreset() == ZONE_CUSTOM) {
                RadialZone z = zoneManager.getDeadCustom();
                if (idx++ == menuSelection) { z.minDist += dir * 100; if(z.minDist < 0) z.minDist=0; zoneManager.setDeadCustom(z); return; }
                if (idx++ == menuSelection) { z.maxDist += dir * 100; if(z.maxDist < z.minDist) z.maxDist=z.minDist; zoneManager.setDeadCustom(z); return; }
                if (idx++ == menuSelection) { z.minAngle += dir * 5; if(z.minAngle < -90) z.minAngle=-90; zoneManager.setDeadCustom(z); return; }
                if (idx++ == menuSelection) { z.maxAngle += dir * 5; if(z.maxAngle > 90) z.maxAngle=90; zoneManager.setDeadCustom(z); return; }
            }
        }
        else if (activePage == PAGE_DATA) {
            if (idx++ == menuSelection) {
                int tm = (int)telemetryMode + dir;
                if (tm > 4) tm = 0; if (tm < 0) tm = 4;
                telemetryMode = (TelemetryMode)tm;
                return;
            }
            if (idx++ == menuSelection) { sensitivity += dir; if (sensitivity < 1) sensitivity = 10; if (sensitivity > 10) sensitivity = 10; return; }
            if (idx++ == menuSelection) { locationAveraging += dir; if (locationAveraging < 1) locationAveraging = 1; if (locationAveraging > 10) locationAveraging = 10; return; }
            if (idx++ == menuSelection) {
                interpolationAmount += (dir * 0.1f);
                if (interpolationAmount < 0.1f) interpolationAmount = 0.1f;
                if (interpolationAmount > 1.05f) interpolationAmount = 1.0f;
                return;
            }
            if (idx++ == menuSelection) { serialDebugEnabled = !serialDebugEnabled; return; }
        }
    }
};

#endif
