#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <RotaryEncoder.h>
#include <OneButton.h>
#include "E54_Radar.h"
#include "ZoneManager.h"
#include <Preferences.h>

enum AppState {
    STATE_BOOT,
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
    ZoneManager zoneManager;
    Preferences preferences;

    UIManager(TFT_eSPI& display) : tft(display), sprite(&display) {
        state = STATE_RADAR_VIEW;
        menuSelection = 0;

        theme = THEME_ALIEN;
        sweepLineEnabled = true;
        trailLength = 5;
        gridEnabled = true;
        startupAnimEnabled = true;
        sensitivity = 5;
        locationAveraging = 5;
        interpolationAmount = 0.5f;
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
            for (int h=0; h<10; h++) {
                targetHistoryX[i][h] = 120.0f;
                targetHistoryY[i][h] = 240.0f;
            }
        }
    }

    void loadSettings() {
        preferences.begin("radar_ui", false);
        theme = (ThemeStyle)preferences.getInt("theme", THEME_ALIEN);
        sweepLineEnabled = preferences.getBool("sweep", true);
        trailLength = preferences.getInt("trails", 5);
        gridEnabled = preferences.getBool("grid", true);
        startupAnimEnabled = preferences.getBool("startup", true);
        sensitivity = preferences.getInt("sens", 5);
        locationAveraging = preferences.getInt("locAvg", 5);
        // prefs only save ints/strings easily
        interpolationAmount = (float)preferences.getInt("interp", 5) / 10.0f;
        preferences.end();
    }

    void saveSettings() {
        preferences.begin("radar_ui", false);
        preferences.putInt("theme", theme);
        preferences.putBool("sweep", sweepLineEnabled);
        preferences.putInt("trails", trailLength);
        preferences.putBool("grid", gridEnabled);
        preferences.putBool("startup", startupAnimEnabled);
        preferences.putInt("sens", sensitivity);
        preferences.putInt("locAvg", locationAveraging);
        int interDisp = (int)(interpolationAmount * 10.0f + 0.5f);
        preferences.putInt("interp", interDisp);
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
            menuSelection = 0;
            menuOverlayY = 0;
        } else if (state == STATE_MENU) {
            if (menuSelection == maxMenuSelection) {
                saveSettings();
                state = STATE_RADAR_VIEW;
            } else if (menuSelection == maxMenuSelection - 1) {
                actionRequested = 1;
                saveSettings();
                state = STATE_RADAR_VIEW;
            } else {
                state = STATE_MENU_EDIT;
            }
        } else if (state == STATE_MENU_EDIT) {
            saveSettings();
            state = STATE_MENU;
        }
    }

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

                float diffX = (float)targetGoalX[i] - targetCurrentX[i];
                float diffY = (float)targetGoalY[i] - targetCurrentY[i];

                if (abs(diffX) < 0.5f && abs(diffY) < 0.5f) {
                    targetCurrentX[i] = (float)targetGoalX[i];
                    targetCurrentY[i] = (float)targetGoalY[i];
                } else {
                    targetCurrentX[i] += diffX * interpolationAmount;
                    targetCurrentY[i] += diffY * interpolationAmount;
                }
            } else if (lastTargetActive[i]) {
                for (int h = 0; h < 10; h++) {
                    targetHistoryX[i][h] = -100;
                    targetHistoryY[i][h] = -100;
                }
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
                int tx = 120 + 180 * cos(tr);
                int ty = 240 + 180 * sin(tr);
                uint8_t alpha = 255 - ((a * 255) / 30);
                uint16_t trailCol = sprite.alphaBlend(alpha, sweepColor, TFT_BLACK);
                sprite.drawLine(120, 240, tx, ty, trailCol);
            }
        }

        for (int i = 0; i < 3; i++) {
            if (targetActive[i]) {
                int cx = (int)targetCurrentX[i];
                int cy = (int)targetCurrentY[i];

                uint16_t color;
                if (theme == THEME_MINIMAL) {
                    color = TFT_WHITE;
                } else if (theme == THEME_ALIEN) {
                    if (i == 0) color = sprite.color565(0, 255, 0); // Green
                    else if (i == 1) color = sprite.color565(0, 255, 255); // Cyan
                    else color = sprite.color565(255, 0, 255); // Magenta
                } else {
                    if (i == 0) color = sprite.color565(255, 100, 0); // Orange
                    else if (i == 1) color = sprite.color565(0, 150, 255); // Blue
                    else color = sprite.color565(200, 0, 255); // Purple
                }

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

                if (zoneManager.isWarning(i)) {
                    if ((millis() / 200) % 2 == 0) color = TFT_YELLOW;
                    else color = TFT_RED;
                    sprite.drawCircle(cx, cy, 8, color);
                }

                // Tactical Target Reticles
                if (theme == THEME_ALIEN) {
                    sprite.drawCircle(cx, cy, 6, color);
                    sprite.fillCircle(cx, cy, 2, color);
                    // Add small crosshair lines
                    sprite.drawLine(cx-8, cy, cx-4, cy, color);
                    sprite.drawLine(cx+4, cy, cx+8, cy, color);
                    sprite.drawLine(cx, cy-8, cx, cy-4, color);
                    sprite.drawLine(cx, cy+4, cx, cy+8, color);
                } else if (theme == THEME_MINIMAL) {
                    sprite.fillRect(cx - 3, cy - 3, 7, 7, color);
                } else {
                    // Standard theme tactical reticle
                    sprite.fillCircle(cx, cy, 3, color);
                    sprite.drawLine(cx-6, cy, cx-2, cy, color);
                    sprite.drawLine(cx+2, cy, cx+6, cy, color);
                    sprite.drawLine(cx, cy-6, cx, cy-2, color);
                    sprite.drawLine(cx, cy+2, cx, cy+6, color);
                }

                if (theme != THEME_ALIEN) {
                    sprite.setTextColor(TFT_YELLOW, TFT_BLACK);
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
    int menuSelection;
    int menuOverlayY;
    int maxMenuSelection;
    unsigned long bootStartTime;

    ThemeStyle theme;
    bool sweepLineEnabled;
    int trailLength;
    bool gridEnabled;
    bool startupAnimEnabled;

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

    float targetCurrentX[3];
    float targetCurrentY[3];
    float targetHistoryX[3][10];
    float targetHistoryY[3][10];

    bool lastTargetActive[3];
    int lastDrawnX[3];
    int lastDrawnY[3];


    void drawBootScreen() {
        sprite.fillSprite(TFT_BLACK);
        unsigned long elapsed = millis() - bootStartTime;

        // Progress defines how far the animation has expanded (0 to 180 pixel radius)
        int maxR = (elapsed * 180) / 1000;
        if (maxR > 180) maxR = 180;

        uint16_t gridColor = (theme == THEME_ALIEN) ? sprite.color565(0, 100, 0) : sprite.color565(100, 100, 100);

        // Animated sweeping rings
        for (int r = 60; r <= 180; r += 60) {
            if (maxR >= r) {
                // Draw partial circle based on how far past the ring we are
                int sweepDeg = ((maxR - r) * 180) / 30; // Fast expand
                if (sweepDeg > 360) sweepDeg = 360;

                // Draw expanding arc
                for (int a = -180; a < -180 + sweepDeg; a += 5) {
                    float rad = a * 0.0174533f;
                    sprite.drawPixel(120 + r * cos(rad), 240 + r * sin(rad), gridColor);
                }
            }
        }

        // Animated Crosshairs (Center lines)
        if (maxR > 0) {
            sprite.drawLine(120, 240, 120, 240 - maxR, gridColor);
            sprite.drawLine(120, 240, 120 - maxR, 240, gridColor);
            sprite.drawLine(120, 240, 120 + maxR, 240, gridColor);
        }

        // Boot Text
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
            float cosA = cos(rad);
            float sinA = sin(rad);
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
                        sprite.drawPixel(120 + r * cos(rad), 240 + r * sin(rad), gridColor);
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

        String themeStr = (theme == THEME_STANDARD) ? "Standard" : (theme == THEME_ALIEN ? "Alien" : "Minimal");
        int interDisp = (int)(interpolationAmount * 10.0f + 0.5f);

        String warnStr = (zoneManager.getWarnPreset() == ZONE_OFF) ? "OFF" :
                         (zoneManager.getWarnPreset() == ZONE_CLOSE) ? "CLOSE" :
                         (zoneManager.getWarnPreset() == ZONE_MEDIUM) ? "MED" :
                         (zoneManager.getWarnPreset() == ZONE_FAR) ? "FAR" : "CUSTOM";

        String deadStr = (zoneManager.getDeadPreset() == ZONE_OFF) ? "OFF" :
                         (zoneManager.getDeadPreset() == ZONE_CLOSE) ? "CLOSE" :
                         (zoneManager.getDeadPreset() == ZONE_MEDIUM) ? "MED" :
                         (zoneManager.getDeadPreset() == ZONE_FAR) ? "FAR" : "CUSTOM";

        String items[24];
        int numItems = 0;

        items[numItems++] = "Theme: " + themeStr;
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

        items[numItems++] = "Sweep Line: " + String(sweepLineEnabled ? "ON" : "OFF");
        items[numItems++] = "Trails: " + String(trailLength);
        items[numItems++] = "Grid: " + String(gridEnabled ? "ON" : "OFF");
        items[numItems++] = "Boot Anim: " + String(startupAnimEnabled ? "ON" : "OFF");
        items[numItems++] = "Loc Avg: " + String(locationAveraging);
        items[numItems++] = "Sensitivity: " + String(sensitivity);
        items[numItems++] = "Smoothing: " + String(interDisp);
        items[numItems++] = "[ Reset Tracking ]";
        items[numItems++] = "[ Exit Menu ]";

        maxMenuSelection = numItems - 1;

        int startIdx = max(0, menuSelection - 2);
        if (startIdx > numItems - 5) startIdx = max(0, numItems - 5);

        for (int i = 0; i < 5; i++) {
            int idx = startIdx + i;
            if (idx >= numItems) break;

            int yPos = 10 + i * 20;

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

    void executeMenuEdit(int dir) {
        int idx = 0;
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

        if (idx++ == menuSelection) { sweepLineEnabled = !sweepLineEnabled; return; }
        if (idx++ == menuSelection) { trailLength += dir; if (trailLength < 0) trailLength = 0; if (trailLength > 10) trailLength = 10; return; }
        if (idx++ == menuSelection) { gridEnabled = !gridEnabled; return; }
        if (idx++ == menuSelection) { startupAnimEnabled = !startupAnimEnabled; return; }
        if (idx++ == menuSelection) { locationAveraging += dir; if (locationAveraging < 1) locationAveraging = 1; if (locationAveraging > 10) locationAveraging = 10; return; }
        if (idx++ == menuSelection) { sensitivity += dir; if (sensitivity < 1) sensitivity = 1; if (sensitivity > 10) sensitivity = 10; return; }
        if (idx++ == menuSelection) {
            interpolationAmount += (dir * 0.1f);
            if (interpolationAmount < 0.1f) interpolationAmount = 0.1f;
            if (interpolationAmount > 1.05f) interpolationAmount = 1.0f;
            return;
        }
    }
};

#endif
