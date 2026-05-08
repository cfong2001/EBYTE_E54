#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <RotaryEncoder.h>
#include <OneButton.h>
#include <Preferences.h>
#include "E54_Radar.h"
#include "ZoneManager.h"

#define themeBg 0x1082 // #121315
#define themePrimary 0x06DD // #00dbe9
#define themeDanger 0xFDB5 // #ffb4ab
#define themeSuccess 0x2F20 // #2ae500
#define themeWarning 0xFDCA // #ffb950

enum AppState {
    STATE_BOOT,
    STATE_RADAR_VIEW,
    STATE_MENU,
    STATE_MENU_EDIT,
    STATE_GUIDE,
    STATE_IMPORTING,
    STATE_FALLBACK
};

enum MenuPage {
    PAGE_MAIN,
    PAGE_VISUALS,
    PAGE_ZONES,
    PAGE_DATA,
    PAGE_DEV
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
    ZoneManager zoneManager;
    Preferences preferences;
    bool devRiskAccepted = false;
    bool motionCompEnabled = true;
    bool passthroughMode = true;
    bool showStdDev = false;
    int uiTextSize = 1;

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
            targetStdDev[i] = 0.0f;

            for (int h=0; h<10; h++) {
                targetHistoryX[i][h] = 120.0f;
                targetHistoryY[i][h] = 240.0f;
            }
        }
    }




    // Structure to map preferences strings to integer values
    struct IntPrefMapping {
        const char* key;
        int* valuePtr;
        int defaultVal;
    };

    // Structure to map preferences strings to boolean values
    struct BoolPrefMapping {
        const char* key;
        bool* valuePtr;
        bool defaultVal;
    };

    void loadSettings() {
        preferences.begin("radar_ui", false);

        int tempTheme, tempIcon, tempTelemetryMode, tempInterp;
        IntPrefMapping intMappings[] = {
            {"theme", &tempTheme, THEME_ALIEN},
            {"icon", &tempIcon, ICON_SMART},
            {"trails", &trailLength, 5},
            {"tData", &tempTelemetryMode, TELEMETRY_OFF},
            {"textSize", &uiTextSize, 1},
            {"sens", &sensitivity, 5},
            {"locAvg", &locationAveraging, 5},
            {"interp", &tempInterp, 5}
        };

        BoolPrefMapping boolMappings[] = {
            {"sweep", &sweepLineEnabled, true},
            {"grid", &gridEnabled, true},
            {"startup", &startupAnimEnabled, true},
            {"simSwp", &simulatedSweep, false},
            {"showStd", &showStdDev, false}
        };

        for (const auto& mapping : intMappings) {
            *(mapping.valuePtr) = preferences.getInt(mapping.key, mapping.defaultVal);
        }

        for (const auto& mapping : boolMappings) {
            *(mapping.valuePtr) = preferences.getBool(mapping.key, mapping.defaultVal);
        }

        theme = (ThemeStyle)tempTheme;
        targetIcon = (TargetIcon)tempIcon;
        telemetryMode = (TelemetryMode)tempTelemetryMode;
        interpolationAmount = (float)tempInterp / 10.0f;

        preferences.end();
    }

    void saveSettings() {
        preferences.begin("radar_ui", false);

        int tempTheme = theme;
        int tempIcon = targetIcon;
        int tempTelemetryMode = telemetryMode;
        int tempInterp = (int)(interpolationAmount * 10.0f + 0.5f);

        IntPrefMapping intMappings[] = {
            {"theme", &tempTheme, 0},
            {"icon", &tempIcon, 0},
            {"trails", &trailLength, 0},
            {"tData", &tempTelemetryMode, 0},
            {"textSize", &uiTextSize, 0},
            {"sens", &sensitivity, 0},
            {"locAvg", &locationAveraging, 0},
            {"interp", &tempInterp, 0}
        };

        BoolPrefMapping boolMappings[] = {
            {"sweep", &sweepLineEnabled, false},
            {"grid", &gridEnabled, false},
            {"startup", &startupAnimEnabled, false},
            {"simSwp", &simulatedSweep, false},
            {"showStd", &showStdDev, false}
        };

        for (const auto& mapping : intMappings) {
            preferences.putInt(mapping.key, *(mapping.valuePtr));
        }

        for (const auto& mapping : boolMappings) {
            preferences.putBool(mapping.key, *(mapping.valuePtr));
        }

        preferences.end();
    }


    void init() {
        zoneManager.loadSettings();
        loadSettings();

        Serial.println("[UI] tft.init() start");
        tft.init();
        Serial.println("[UI] tft.init() done");

        // ST7789 boots in sleep mode — must explicitly wake it.
        // tft.init() sends the init sequence but RST timing variance can
        // leave the display in sleep. Force the wake sequence here.
        tft.writecommand(0x11); // SLPOUT - exit sleep mode
        delay(150);             // ST7789 datasheet: min 120ms after SLPOUT
        tft.writecommand(0x29); // DISPON - turn display on
        delay(10);

        tft.setRotation(0);  // portrait 240x320, matches sprite dimensions
        tft.fillScreen(TFT_BLACK); // Clear screen
        Serial.println("[UI] Screen filled BLACK");

#ifdef TFT_BL
        Serial.printf("[UI] Backlight pin %d -> %s\n", TFT_BL,
                      TFT_BACKLIGHT_ON == HIGH ? "HIGH" : "LOW");
        pinMode(TFT_BL, OUTPUT);
        digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
        Serial.println("[UI] Backlight set");
#else
        Serial.println("[UI] WARNING: TFT_BL not defined");
#endif

        bool spriteOk = sprite.createSprite(240, 320);
        Serial.printf("[UI] createSprite(240,320): %s  ptr=%p\n",
                      spriteOk ? "OK" : "FAILED", sprite.getPointer());
        sprite.setSwapBytes(true);

        if (startupAnimEnabled) {
            state = STATE_BOOT;
            bootStartTime = millis();
        } else {
            state = STATE_RADAR_VIEW;
        }
        Serial.println("[UI] init() complete");
    }

    void handleEncoder(int dir) {
        if (state == STATE_GUIDE) {
            guidePage += dir;
            if (guidePage < 0) guidePage = 0;
            if (guidePage > 2) guidePage = 2;
            return;
        }
        if (state == STATE_MENU) {
            menuSelection += dir;
            if (menuSelection > maxMenuSelection) menuSelection = 0;
            if (menuSelection < 0) menuSelection = maxMenuSelection;
        } else if (state == STATE_MENU_EDIT) {
            executeMenuEdit(dir);
        }
    }


    void handleButtonLongPress() {
        if (state == STATE_MENU) {
            showTooltip = true;
        }
    }

    void handleButton() {

        if (state == STATE_IMPORTING) {
            state = STATE_MENU; // Cancel import
        } else if (state == STATE_FALLBACK) {
            actionRequested = 3; // Confirm fallback
            state = STATE_RADAR_VIEW;
        } else if (state == STATE_RADAR_VIEW) {
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



    void setTargetMotion(int index, float vx, float vy, float ax, float ay, float stdDev = 0.0f) {
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
            targetCoasting[i] = targets[i].isCoasting;

            if (targets[i].active) {
                int16_t absSpeed = abs(targets[i].speed);
                if (absSpeed < sensitivity && sensitivity > 1) {
                    targetActive[i] = false;
                } else {
                    targetGoalX[i] = 120 + (targets[i].x * 120 / 5000);
                    targetGoalY[i] = 320 - (targets[i].y * 320 / 5000);

                    rawTargetX[i] = targets[i].x;
                    rawTargetY[i] = targets[i].y;
                    rawTargetSpeed[i] = targets[i].speed;

                    if (targetGoalX[i] < 0 || targetGoalX[i] >= 240 || targetGoalY[i] < 0 || targetGoalY[i] >= 320) {
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

    void drawGuideScreen() {
        sprite.fillSprite(themeBg);
        sprite.setTextColor(themePrimary, themeBg);
        sprite.setTextSize(uiTextSize);
        sprite.setCursor(10, 10);

        if (guidePage == 0) {
            sprite.print("GUIDE 1/3: TARGETS");
            sprite.setCursor(10, 40);
            sprite.setTextColor(TFT_WHITE, themeBg);
            sprite.print("The radar tracks up");
            sprite.setCursor(10, 55);
            sprite.print("to 3 targets at once.");

            // Draw dummy targets
            sprite.drawCircle(30, 90, 8, themePrimary);
            sprite.setCursor(50, 85); sprite.print("Moving target");

            sprite.drawRect(22, 112, 16, 16, themeWarning);
            sprite.setCursor(50, 115); sprite.print("Stationary target");

            sprite.fillTriangle(30, 140, 22, 156, 38, 156, themeSuccess);
            sprite.setCursor(50, 145); sprite.print("Selected target");



        } else if (guidePage == 1) {
            sprite.print("GUIDE 2/3: CONTROLS");
            sprite.setCursor(10, 40);
            sprite.setTextColor(TFT_WHITE, themeBg);
            sprite.print("Navigate via the dial:");

            sprite.setCursor(20, 80); sprite.print("- TURN: Scroll/Adjust");
            sprite.setCursor(20, 110); sprite.print("- PRESS: Select/Enter");
            sprite.setCursor(20, 140); sprite.print("- HOLD: Info Tooltips");



        } else if (guidePage == 2) {
            sprite.print("GUIDE 3/3: ZONES");
            sprite.setCursor(10, 40);
            sprite.setTextColor(TFT_WHITE, themeBg);
            sprite.print("Zones highlight targets.");

            // Draw a mini radar zone
            sprite.drawCircle(120, 160, 40, themeWarning);
            sprite.setCursor(10, 80); sprite.setTextColor(themeWarning, themeBg);
            sprite.print("Warning Zone (Amber)");

            sprite.drawCircle(120, 160, 20, themeDanger);
            sprite.setCursor(10, 100); sprite.setTextColor(themeDanger, themeBg);
            sprite.print("Dead Zone (Hidden)");


        }

        sprite.setCursor(10, 200);
        sprite.setTextColor(themeWarning, themeBg);
        sprite.print("[Turn] Next  [Press] Exit");

        int dotStartX = 110;
        for (int i = 0; i < 3; i++) {
            if (i == guidePage) {
                sprite.fillCircle(dotStartX + i * 10, 225, 3, themePrimary);
            } else {
                sprite.drawCircle(dotStartX + i * 10, 225, 3, sprite.alphaBlend(100, themePrimary, themeBg));
            }
        }

        sprite.pushSprite(0, 0);
    }

    void renderLoop() {
        if (state == STATE_BOOT) {
            drawBootScreen();
            return;
        }
        if (state == STATE_GUIDE) {
            drawGuideScreen();
            return;
        }

        advanceTargets();

        sprite.fillSprite(themeBg);

        if (theme == THEME_MINIMAL) {
            if (gridEnabled) {
                sprite.drawCircle(120, 240, 180, 0x18E3);
                sprite.fillCircle(120, 240, 4, 0x18E3);
            }
        } else {
            drawRadarBackground();
        }

        drawZones();
        drawSweepLine();

        bool anyActive = false;
        for (int i = 0; i < 3; i++) {
            if (targetActive[i] && simAlpha[i] > 0.01f) {
                anyActive = true;
            }
            drawTarget(i);
        }

        if (!anyActive) {
            float pulse = (sinf(millis() / 800.0f) + 1.0f) * 0.5f;
            uint16_t emptyColor = sprite.alphaBlend((uint8_t)(pulse * 150.0f) + 50, themePrimary, themeBg);
            sprite.setTextColor(emptyColor, themeBg);
            sprite.setTextSize(uiTextSize);
            sprite.setCursor(85, 116);
            sprite.print("NO CONTACTS");
        }

        drawHUD();

        if (state == STATE_MENU || state == STATE_MENU_EDIT) {
            drawMenuOverlay();
        } else if (state == STATE_IMPORTING) {
            sprite.fillRect(10, 100, 220, 40, themeWarning);
            sprite.setTextColor(themeBg, themeWarning);
            sprite.setCursor(20, 110);
            sprite.print("WAITING FOR CONFIG...");
            sprite.setCursor(20, 125);
            sprite.print("[PRESS BUTTON TO CANCEL]");
        } else if (state == STATE_FALLBACK) {
            sprite.fillRect(10, 90, 220, 60, themeDanger);
            sprite.setTextColor(TFT_WHITE, themeDanger);
            sprite.setCursor(20, 100);
            sprite.print("NEW CONFIG LOADED");
            sprite.setCursor(20, 115);
            sprite.print("PRESS BUTTON TO KEEP");
            sprite.setCursor(20, 130);
            sprite.print("OR WAIT TO REVERT...");
        }

        tft.startWrite();
        sprite.pushSprite(0, 0);  // PSRAM-safe: no DMA required
        tft.endWrite();
    }
    int getSensitivity() { return sensitivity; }
    int getLocationAveraging() { return locationAveraging; }

    int consumeAction() {
        int act = actionRequested;
        actionRequested = 0;
        return act;
    }

    void logStateToSerial() {
        Serial.printf("State: %d, Page: %d | Danger: %.2f\n", state, activePage, zoneManager.getDangerLevel());
        for (int i = 0; i < 3; i++) {
            if (targetActive[i]) {
                Serial.printf("  T%d: [%d, %d] Spd:%d\n", i+1, rawTargetX[i], rawTargetY[i], rawTargetSpeed[i]);
            }
        }
    }

    void advanceTargets() {
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

                float t = 0.03f; // DT ~30ms render loop
                float t_sq_half = (t * t) * 0.5f;

                float curveForwardX = (targetVelX[i] * t) + (targetAccX[i] * t_sq_half);
                float curveForwardY = (targetVelY[i] * t) + (targetAccY[i] * t_sq_half);

                if (targetCoasting[i]) {
                    // Seamless Coasting: Do not interpolate towards the static 'targetGoal' from the dropped packet.
                    // Just push the sub-frame curve forward identically to the backend tracker.
                    targetCurrentX[i] += curveForwardX;
                    targetCurrentY[i] += curveForwardY;

                    // Keep targetGoal updated so when sensor regains lock, the visual diffX isn't huge.
                    targetGoalX[i] = (int)targetCurrentX[i];
                    targetGoalY[i] = (int)targetCurrentY[i];
                } else {
                    float diffX = (float)targetGoalX[i] - targetCurrentX[i];
                    float diffY = (float)targetGoalY[i] - targetCurrentY[i];

                    if (fabsf(diffX) < 0.5f && fabsf(diffY) < 0.5f) {
                        targetCurrentX[i] = (float)targetGoalX[i];
                        targetCurrentY[i] = (float)targetGoalY[i];
                    } else {
                        targetCurrentX[i] += (diffX * interpolationAmount) + curveForwardX;
                        targetCurrentY[i] += (diffY * interpolationAmount) + curveForwardY;
                    }
                }

                if (simulatedSweep) {
                    float targetRad = atan2f(rawTargetX[i], rawTargetY[i]);
                    int targetDeg = (int)(targetRad * 180.0f / PI);

                    int visualAngle = sweepAngle - 90;

                    if (abs(visualAngle - targetDeg) < 5) {
                        simAlpha[i] = 1.0f;
                    }
                    simAlpha[i] -= 0.03f;
                    if (simAlpha[i] < 0.0f) simAlpha[i] = 0.0f;
                } else {
                    simAlpha[i] = 1.0f;
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
    }

    void drawSweepLine() {
        if (sweepLineEnabled && theme != THEME_MINIMAL) {
            sweepAngle = (sweepAngle + 4) % 180;
            uint16_t sweepColor = (theme == THEME_ALIEN) ? themePrimary : TFT_DARKGREY;
            for (int a = 0; a < 30; a += 2) {
                float tr = (sweepAngle - a - 180) * 0.0174533f;
                int tx = 120 + 180 * cosf(tr);
                int ty = 320 + 180 * sinf(tr);
                uint8_t alpha = 255 - ((a * 255) / 30);
                uint16_t trailCol = sprite.alphaBlend(alpha, sweepColor, themeBg);
                sprite.drawLine(120, 320, tx, ty, trailCol);
            }
        }
    }


private:
    void drawTargetTrail(int i, uint8_t currentAlpha, uint16_t baseColor) {
        if (trailLength > 0) {
            for (int h = 0; h < trailLength; h++) {
                int hx = (int)targetHistoryX[i][h];
                int hy = (int)targetHistoryY[i][h];
                if (hx > 0 && hy > 0) {
                    uint8_t t_alpha = (currentAlpha * (trailLength - h)) / trailLength;
                    uint16_t tColor = sprite.alphaBlend(t_alpha, baseColor, themeBg);
                    int tr = max(1, 4 - (h / 2));
                    sprite.fillCircle(hx, hy, tr, tColor);
                }
            }
        }
    }

    void drawTargetWarning(int i, int cx, int cy, uint8_t currentAlpha) {
        if (zoneManager.isWarning(i)) {
            float pulse = (sinf(millis() / 150.0f) + 1.0f) * 0.5f;
            uint8_t blendRatio = (uint8_t)(pulse * 255.0f);
            uint16_t blendColor = sprite.alphaBlend(blendRatio, themeDanger, themeWarning);
            uint16_t wCol = sprite.alphaBlend(currentAlpha, blendColor, themeBg);
            int pr = 8 + (int)(pulse * 2.0f);
            sprite.drawCircle(cx, cy, pr, wCol);
        }
        float danger = zoneManager.getTargetDangerLevel(i);
        if (danger > 0.01f) {
            uint16_t dangerColor = sprite.alphaBlend((uint8_t)(danger * 255.0f), themeDanger, themeWarning);
            uint16_t wCol = sprite.alphaBlend(currentAlpha, dangerColor, themeBg);

            float pulseSpeed = 300.0f - (danger * 200.0f);
            // ⚡ Bolt: Use single-precision sinf() to avoid implicit double conversion
            // inside 30Hz display rendering loop, saving CPU cycles on ESP32 FPU.
            float pulse = (sinf(millis() / pulseSpeed) + 1.0f) * 0.5f;
            int r = 6 + (int)(pulse * 4.0f * danger);

            sprite.drawCircle(cx, cy, r, wCol);
        }
    }

    void drawTargetIcon(int i, int cx, int cy, uint16_t color) {
        if (targetIcon == ICON_CIRCLE) {
            sprite.fillCircle(cx, cy, 4, color);
            if (!targetCoasting[i]) sprite.drawCircle(cx, cy, 5, TFT_WHITE);
        }
        else if (targetIcon == ICON_SQUARE) {
            sprite.fillRect(cx - 3, cy - 3, 7, 7, color);
            if (!targetCoasting[i]) sprite.drawRect(cx - 4, cy - 4, 9, 9, TFT_WHITE);
        }
        else if (targetIcon == ICON_TRIANGLE) {
            sprite.fillTriangle(cx, cy - 5, cx - 4, cy + 3, cx + 4, cy + 3, color);
            if (!targetCoasting[i]) sprite.drawTriangle(cx, cy - 6, cx - 5, cy + 4, cx + 5, cy + 4, TFT_WHITE);
        }
        else if (targetIcon == ICON_SMART) {
            sprite.drawCircle(cx, cy, 3, color);
            if (!targetCoasting[i]) sprite.drawCircle(cx, cy, 4, TFT_WHITE);

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
    }

    void drawTarget(int i) {
        if (targetActive[i] && simAlpha[i] > 0.01f) {
            int cx = (int)targetCurrentX[i];
            int cy = (int)targetCurrentY[i];

            uint16_t baseColor;
            if (theme == THEME_MINIMAL) {
                baseColor = TFT_WHITE;
            } else if (theme == THEME_ALIEN) {
                baseColor = themePrimary;
            } else {
                if (i == 0) baseColor = themeWarning;
                else if (i == 1) baseColor = themePrimary;
                else baseColor = themeDanger;
            }

            uint8_t currentAlpha = (uint8_t)(simAlpha[i] * 255.0f);
            uint16_t color = sprite.alphaBlend(currentAlpha, baseColor, themeBg);

            drawTargetTrail(i, currentAlpha, baseColor);

            // Draw StdDev Visualization Circle
            if (showStdDev && targetStdDev[i] > 1.0f) {
                // Convert millimeter stdDev to screen pixels (approx 120 pixels per 5000 mm)
                float screenRadius = targetStdDev[i] * (120.0f / 5000.0f);
                if (screenRadius < 2.0f) screenRadius = 2.0f;
                if (screenRadius > 120.0f) screenRadius = 120.0f;
                uint16_t devColor = sprite.alphaBlend(100, baseColor, themeBg); // subtle wireframe
                sprite.drawCircle(cx, cy, (int)screenRadius, devColor);
            }

            drawTargetWarning(i, cx, cy, currentAlpha);

            drawTargetIcon(i, cx, cy, color);

            if (theme != THEME_ALIEN && telemetryMode != TELEMETRY_OFF) {
                sprite.setTextColor(color, themeBg);

                sprite.setCursor(cx + 8, cy - 12);

                if (telemetryMode == TELEMETRY_DIST_ANG) {
                    float dist_m = sqrtf((long)rawTargetX[i]*rawTargetX[i] + (long)rawTargetY[i]*rawTargetY[i]) / 1000.0f;
                    int angle = (int)(atan2f((float)rawTargetX[i], (float)rawTargetY[i]) * 180.0f / PI);
                    sprite.printf("%.1fm %d", dist_m, angle);
                    sprite.printf("%.1fm %ddeg", dist_m, angle);
                } else if (telemetryMode == TELEMETRY_VELOCITY) {
                    float speed_ms = (float)rawTargetSpeed[i] / 10.0f;
                    sprite.printf("%.1fm/s", speed_ms);
                } else if (telemetryMode == TELEMETRY_RAW) {
                    sprite.printf("%dmm,%dmm", rawTargetX[i], rawTargetY[i]);
                } else if (telemetryMode == TELEMETRY_ALL) {
                    float dist_m = sqrtf((long)rawTargetX[i]*rawTargetX[i] + (long)rawTargetY[i]*rawTargetY[i]) / 1000.0f;
                    int angle = (int)(atan2f((float)rawTargetX[i], (float)rawTargetY[i]) * 180.0f / PI);
                    float speed_ms = (float)rawTargetSpeed[i] / 10.0f;
                    sprite.printf("T%d %.1fm %d", i+1, dist_m, angle);
                    sprite.printf("T%d %.1fm %ddeg", i+1, dist_m, angle);
                    sprite.setCursor(cx + 8, cy - 2);
                    sprite.printf("%.1fm/s", speed_ms);
                }
            } else if (theme != THEME_ALIEN && telemetryMode == TELEMETRY_OFF) {
                sprite.setTextColor(color, themeBg);
                sprite.setCursor(cx + 8, cy - 8);
                sprite.printf("T%d", i + 1);
            }
        }
    }

    void drawHUD() {
        sprite.fillRect(0, 0, 240, 16, themeBg);
        sprite.fillRect(0, 304, 240, 16, themeBg);

        sprite.drawLine(0, 16, 240, 16, sprite.alphaBlend(100, themePrimary, themeBg));
        sprite.drawLine(0, 304, 240, 304, sprite.alphaBlend(100, themePrimary, themeBg));

        sprite.setTextColor(themePrimary, themeBg);
        sprite.setTextSize(uiTextSize);
        sprite.setCursor(5, 4);
        sprite.print("((o)) RADAR_V1.0");

        sprite.setCursor(215, 4);
        sprite.print("BAT");

        sprite.setCursor(5, 308);
        if (state == STATE_RADAR_VIEW) {
            sprite.print("[VIEW]  MENU");
        } else if (state == STATE_MENU_EDIT) {
            sprite.print(" EDIT  [SAVE]");
        } else {
            sprite.print(" VIEW  [MENU]");
        }

        if (theme != THEME_MINIMAL) {
            if (anchorValid) {
                sprite.setTextColor(themePrimary, themeBg);
                sprite.setCursor(5, 5);
                sprite.printf("Anchor: (%dmm, %dmm)", anchorX, anchorY);
            } else {
                sprite.setTextColor(themeDanger, themeBg);
                sprite.setCursor(5, 5);
                sprite.printf("No Anchor");
            }
        }
    }

private:
    TFT_eSPI& tft;
    TFT_eSprite sprite;
public:
public:
    AppState state;
    MenuPage activePage;
    int menuSelection;
    int menuOverlayY;
    int maxMenuSelection;
    int guidePage = 0;
    bool showTooltip = false;
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
    bool targetCoasting[3];
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
    float targetStdDev[3];

    bool lastTargetActive[3];
    int lastDrawnX[3];
    int lastDrawnY[3];

    void drawBootScreen() {
        sprite.fillSprite(themeBg);
        unsigned long elapsed = millis() - bootStartTime;

        int maxR = (elapsed * 180) / 1000;
        if (maxR > 180) maxR = 180;

        uint16_t gridColor = (theme == THEME_ALIEN) ? themePrimary : themePrimary;

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

        sprite.setTextColor(themePrimary, themeBg);
        sprite.setTextSize(uiTextSize);
        if (elapsed < 300) sprite.setCursor(100, 120), sprite.print("INIT");
        else if (elapsed < 600) sprite.setCursor(90, 120), sprite.print("CALIBRATING");
        else if (elapsed < 1000) sprite.setCursor(95, 120), sprite.print("SCANNING...");

        tft.startWrite(); sprite.pushSprite(0, 0); tft.endWrite();  // PSRAM-safe push, full 240x320

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
            drawRadialWedge(z.minDist, z.maxDist, z.minAngle, z.maxAngle, themeDanger);
        }
        if (zoneManager.getWarnPreset() != ZONE_OFF) {
            RadialZone z = zoneManager.getActiveWarnZone();
            float danger = zoneManager.getDangerLevel();
            uint16_t baseColor = themeBg;
            uint16_t activeColor = themePrimary;
            uint8_t alpha = (uint8_t)(danger * 255.0f);
            uint16_t dangerColor = sprite.alphaBlend(alpha, themeDanger, themeWarning);
            uint16_t wedgeColor = sprite.alphaBlend(alpha, dangerColor, baseColor);
            drawRadialWedge(z.minDist, z.maxDist, z.minAngle, z.maxAngle, wedgeColor);
        }
    }


    void drawRadarBackground() {
        uint16_t gridColor = (theme == THEME_ALIEN) ? themePrimary : TFT_DARKGREY;
        gridColor = sprite.alphaBlend(80, gridColor, themeBg); // Dimmer lines
        if (gridEnabled) {
            // Tactical crosshair
            sprite.drawLine(0, 120, 240, 120, gridColor);
            sprite.drawLine(120, 16, 120, 304, gridColor); // Avoid drawing over top/bottom bars

            // Faint concentric circles
            for (int r = 40; r <= 100; r += 30) {
                sprite.drawRect(120 - r, 120 - r, r * 2, r * 2, sprite.alphaBlend(50, themePrimary, themeBg));
            }
            if (theme == THEME_ALIEN) {
                for (int r=60; r<=180; r+=60) {
                    for (int a=0; a<=180; a+=5) {
                        float rad = (a - 180) * 0.0174533f;
                        sprite.drawPixel(120 + r * cosf(rad), 240 + r * sinf(rad), gridColor);
                    }
                }
            } else {
                sprite.drawRect(60, 180, 120, 120, gridColor);
                sprite.drawRect(0, 120, 240, 240, gridColor);
            }

            // Ticks along axes
            for (int r = 30; r <= 90; r += 30) {
                sprite.drawLine(120 - 3, 120 - r, 120 + 3, 120 - r, gridColor); // Vertical axis ticks
                sprite.drawLine(120 - r, 120 - 3, 120 - r, 120 + 3, gridColor); // Horizontal axis ticks
                sprite.drawLine(120 + r, 120 - 3, 120 + r, 120 + 3, gridColor);
            }
        }
    }


    void populateMainMenu(String* items, int& numItems) {
        sprite.setTextColor(themePrimary, themeBg);
        sprite.setCursor(15, 5); sprite.print("CONFIG MENU");

        items[numItems++] = "VISUAL SETTINGS";
        items[numItems++] = "  [DISPLAY/HUD]";
        items[numItems++] = "ZONE CONFIG";
        items[numItems++] = "  [BOUNDARIES]";
        items[numItems++] = "TARGET DATA";
        items[numItems++] = "  [GAIN/FILTER]";
        items[numItems++] = "DEV OPTIONS";
        items[numItems++] = "USER GUIDE";
        items[numItems++] = "[ Exit Menu ]";
    }

    void populateVisualsMenu(String* items, int& numItems) {
        sprite.setTextColor(themePrimary, themeBg);
        sprite.setCursor(15, 5); sprite.print("--- VISUAL SETTINGS ---");

        String themeStr = (theme == THEME_STANDARD) ? "Standard" : (theme == THEME_ALIEN ? "Alien" : "Minimal");
        String iconStr = (targetIcon == ICON_CIRCLE) ? "CIRCLE" :
                         (targetIcon == ICON_SQUARE) ? "SQUARE" :
                         (targetIcon == ICON_TRIANGLE) ? "TRIANGLE" : "SMART";
        items[numItems++] = "<- Back";
        items[numItems++] = "Theme: " + themeStr;
        items[numItems++] = "Icon: " + iconStr;
        items[numItems++] = "Text Size: " + String(uiTextSize);
        items[numItems++] = "Sweep Line: " + String(sweepLineEnabled ? "ON" : "OFF");
        items[numItems++] = "Sweep Mode: " + String(simulatedSweep ? "SIMULATED" : "VISUAL");
        items[numItems++] = "Trails: " + String(trailLength);
        items[numItems++] = "Grid: " + String(gridEnabled ? "ON" : "OFF");
        items[numItems++] = "Boot Anim: " + String(startupAnimEnabled ? "ON" : "OFF");
    }

    void populateZonesMenu(String* items, int& numItems) {
        sprite.setTextColor(themePrimary, themeBg);
        sprite.setCursor(15, 5); sprite.print("--- ZONE CONFIG ---");

        String warnStr = (zoneManager.getWarnPreset() == ZONE_OFF) ? "OFF" :
                         (zoneManager.getWarnPreset() == ZONE_CLOSE) ? "CLOSE" :
                         (zoneManager.getWarnPreset() == ZONE_MEDIUM) ? "MED" :
                         (zoneManager.getWarnPreset() == ZONE_FAR) ? "FAR" : "CUSTOM";

        String deadStr = (zoneManager.getDeadPreset() == ZONE_OFF) ? "OFF" :
                         (zoneManager.getDeadPreset() == ZONE_CLOSE) ? "CLOSE" :
                         (zoneManager.getDeadPreset() == ZONE_MEDIUM) ? "MED" :
                         (zoneManager.getDeadPreset() == ZONE_FAR) ? "FAR" : "CUSTOM";

        items[numItems++] = "<- Back";
        items[numItems++] = "Warn Zone: " + warnStr;
        if (zoneManager.getWarnPreset() == ZONE_CUSTOM) {
            items[numItems++] = " W-MinD: " + String(zoneManager.getWarnCustom().minDist) + "mm";
            items[numItems++] = " W-MaxD: " + String(zoneManager.getWarnCustom().maxDist) + "mm";
            items[numItems++] = " W-MinA: " + String(zoneManager.getWarnCustom().minAngle) + "deg";
            items[numItems++] = " W-MaxA: " + String(zoneManager.getWarnCustom().maxAngle) + "deg";
        }
        if (zoneManager.getWarnPreset() != ZONE_OFF) {
            items[numItems++] = "Warn Fuzz: " + String(zoneManager.getFuzzingThreshold()) + "%";
            items[numItems++] = "Warn Time: " + String(zoneManager.getHistoryWindow() * 100) + "ms";
        }

        items[numItems++] = "Dead Zone: " + deadStr;
        if (zoneManager.getDeadPreset() == ZONE_CUSTOM) {
            items[numItems++] = " D-MinD: " + String(zoneManager.getDeadCustom().minDist) + "mm";
            items[numItems++] = " D-MaxD: " + String(zoneManager.getDeadCustom().maxDist) + "mm";
            items[numItems++] = " D-MinA: " + String(zoneManager.getDeadCustom().minAngle) + "deg";
            items[numItems++] = " D-MaxA: " + String(zoneManager.getDeadCustom().maxAngle) + "deg";
        }
    }

    void populateDataMenu(String* items, int& numItems) {
        sprite.setTextColor(themePrimary, themeBg);
        sprite.setCursor(15, 5); sprite.print("--- TARGET DATA ---");

        String tDataStr = (telemetryMode == TELEMETRY_OFF) ? "OFF" :
                          (telemetryMode == TELEMETRY_DIST_ANG) ? "DIST/ANG" :
                          (telemetryMode == TELEMETRY_VELOCITY) ? "SPEED" :
                          (telemetryMode == TELEMETRY_RAW) ? "RAW X/Y" : "ALL";
        int interDisp = (int)(interpolationAmount * 10.0f + 0.5f);

        items[numItems++] = "<- Back";
        items[numItems++] = "Telemetry: " + tDataStr;
        items[numItems++] = "Sensitivity: " + String(sensitivity) + " cm/s";
        items[numItems++] = "Loc Avg: " + String(locationAveraging);
        items[numItems++] = "Smoothing: " + String(interDisp);
        items[numItems++] = "[ Reset Tracking ]";
    }

    void populateDevMenu(String* items, int& numItems) {
        sprite.setTextColor(themeDanger, themeBg);
        sprite.setCursor(15, 5); sprite.print("--- DEV OPTIONS ---");

        items[numItems++] = "<- Back";
        items[numItems++] = "Accept Risk? " + String(devRiskAccepted ? "YES" : "NO");
        if (devRiskAccepted) {
            items[numItems++] = "Motion Comp: " + String(motionCompEnabled ? "ON" : "OFF");
            items[numItems++] = "Passthrough: " + String(passthroughMode ? "ON" : "OFF");
            items[numItems++] = "Show StdDev: " + String(showStdDev ? "ON" : "OFF");
            items[numItems++] = "[ FACTORY RESET ]";
            items[numItems++] = "[ EXPORT CONFIG ]";
            items[numItems++] = "[ IMPORT CONFIG ]";
        }
    }

    void populateMenuPage(String* items, int& numItems) {
        if (activePage == PAGE_MAIN) {
            populateMainMenu(items, numItems);
        } else if (activePage == PAGE_VISUALS) {
            populateVisualsMenu(items, numItems);
        } else if (activePage == PAGE_ZONES) {
            populateZonesMenu(items, numItems);
        } else if (activePage == PAGE_DATA) {
            populateDataMenu(items, numItems);
        } else if (activePage == PAGE_DEV) {
            populateDevMenu(items, numItems);
        }
    }

    void drawMenuItems(String* items, int numItems) {
        maxMenuSelection = numItems - 1;

        int startIdx = max(0, menuSelection - 2);
        if (startIdx > numItems - 4) startIdx = max(0, numItems - 4);

        for (int i = 0; i < 4; i++) {
            int idx = startIdx + i;
            if (idx >= numItems) break;

            int yPos = 35 + i * 25;

            if (idx == menuSelection) {
                if (state == STATE_MENU_EDIT) {
                    sprite.fillRect(5, yPos - 4, 230, 24, themeWarning);
                    sprite.setTextColor(themeBg, themeWarning);
                } else {
                    sprite.fillRect(5, yPos - 4, 230, 24, themePrimary);
                    sprite.setTextColor(themeBg, themePrimary);
                }
            } else {
                sprite.setTextColor(TFT_WHITE, themeBg);
            }

            sprite.setCursor(15, yPos);
            sprite.print(items[idx]);
        }

        if (numItems > 4) {
            int sbX = 236;
            int sbY = 35;
            int sbH = 96;
            sprite.drawRect(sbX, sbY, 2, sbH, sprite.alphaBlend(100, themePrimary, themeBg));
            int handleH = max(10, (sbH * 4) / numItems);
            int handleY = sbY + (int)(((float)menuSelection / (numItems - 1)) * (sbH - handleH));
            sprite.fillRect(sbX, handleY, 2, handleH, themePrimary);
            int scrollTrackH = 4 * 25 - 4;
            int scrollTrackY = 35 - 4;
            sprite.drawLine(235, scrollTrackY, 235, scrollTrackY + scrollTrackH, sprite.alphaBlend(100, themePrimary, themeBg));
            int thumbH = max(4, (scrollTrackH * 4) / numItems);
            int thumbY = scrollTrackY + (startIdx * scrollTrackH) / numItems;
            sprite.fillRect(233, thumbY, 5, thumbH, themePrimary);
        }

        if (showTooltip) {
            sprite.fillRect(10, 140, 220, 60, themeBg);
            sprite.drawRect(10, 140, 220, 60, themeWarning);
            sprite.setTextColor(TFT_WHITE, themeBg);
            sprite.setTextSize(uiTextSize);
            sprite.setCursor(15, 145);
            sprite.print("INFO: ");
            sprite.setCursor(15, 160);

            // Simple logic to display something based on item
            if (activePage == PAGE_MAIN) {
                if (menuSelection == 0) sprite.print("Adjust visual themes,");
                else if (menuSelection == 1) sprite.print("icons, and display settings.");
                else if (menuSelection == 2) sprite.print("Configure warning and");
                else if (menuSelection == 3) sprite.print("dead zones.");
                else if (menuSelection == 4) sprite.print("Adjust raw telemetry");
                else if (menuSelection == 5) sprite.print("filtering options.");
                else if (menuSelection == 6) sprite.print("Return to radar view.");
                else sprite.print("Select an option.");
            } else {
                sprite.print("Adjust this setting to");
                sprite.setCursor(15, 175);
                sprite.print("change device behavior.");
            }
        }
    }

    void drawMenuOverlay() {
        if (menuOverlayY < 200) menuOverlayY += 15;

        sprite.fillRect(0, 0, 240, menuOverlayY, sprite.alphaBlend(220, themeBg, TFT_WHITE));
        sprite.drawLine(0, menuOverlayY, 240, menuOverlayY, themePrimary);
        if (menuOverlayY < 200) return;

        sprite.setTextSize(uiTextSize);
        static String items[24];
        int numItems = 0;

        populateMenuPage(items, numItems);
        drawMenuItems(items, numItems);
    }

    void handleMenuClick() {
        if (activePage == PAGE_MAIN) {
            if (menuSelection == 0 || menuSelection == 1) { activePage = PAGE_VISUALS; menuSelection = 0; }
            else if (menuSelection == 2 || menuSelection == 3) { activePage = PAGE_ZONES; menuSelection = 0; }
            else if (menuSelection == 4 || menuSelection == 5) { activePage = PAGE_DATA; menuSelection = 0; }
            else if (menuSelection == 6) { activePage = PAGE_DEV; menuSelection = 0; }
            else if (menuSelection == 7) { state = STATE_GUIDE; guidePage = 0; }
            else if (menuSelection == 8) { state = STATE_RADAR_VIEW; }
        }
        else if (activePage == PAGE_VISUALS) {
            if (menuSelection == 0) { activePage = PAGE_MAIN; menuSelection = 0; }
            else { state = STATE_MENU_EDIT; }
        }
        else if (activePage == PAGE_ZONES) {
            if (menuSelection == 0) { activePage = PAGE_MAIN; menuSelection = 0; }
            else { state = STATE_MENU_EDIT; }
        }
        else if (activePage == PAGE_DEV) {
            if (menuSelection == 0) { activePage = PAGE_MAIN; menuSelection = 0; }
            else if (menuSelection == maxMenuSelection - 1 && devRiskAccepted) { actionRequested = 2; state = STATE_RADAR_VIEW; }
            else if (menuSelection == maxMenuSelection && devRiskAccepted) { state = STATE_IMPORTING; }
            else { state = STATE_MENU_EDIT; }
        }
        else if (activePage == PAGE_DATA) {
            if (menuSelection == 0) { activePage = PAGE_MAIN; menuSelection = 0; }
            else if (menuSelection == maxMenuSelection) { actionRequested = 1; state = STATE_RADAR_VIEW; }
            else { state = STATE_MENU_EDIT; }
        }
    }

    void executeVisualsMenuEdit(int dir, int& idx) {
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
        if (idx++ == menuSelection) { uiTextSize += dir; if (uiTextSize < 1) uiTextSize = 1; if (uiTextSize > 2) uiTextSize = 2; return; }
        if (idx++ == menuSelection) { sweepLineEnabled = !sweepLineEnabled; return; }
        if (idx++ == menuSelection) { simulatedSweep = !simulatedSweep; return; }
        if (idx++ == menuSelection) { trailLength += dir; if (trailLength < 0) trailLength = 0; if (trailLength > 10) trailLength = 10; return; }
        if (idx++ == menuSelection) { gridEnabled = !gridEnabled; return; }
        if (idx++ == menuSelection) { startupAnimEnabled = !startupAnimEnabled; return; }
    }

    void executeZonesMenuEdit(int dir, int& idx) {
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

    void executeDevMenuEdit(int dir, int& idx) {
        if (idx++ == menuSelection) { devRiskAccepted = !devRiskAccepted; return; }
        if (devRiskAccepted) {
            if (idx++ == menuSelection) { motionCompEnabled = !motionCompEnabled; return; }
            if (idx++ == menuSelection) { passthroughMode = !passthroughMode; return; }
            if (idx++ == menuSelection) { showStdDev = !showStdDev; return; }
            if (idx++ == menuSelection) {
                sprite.fillSprite(themeDanger);
                sprite.setTextColor(TFT_WHITE);
                sprite.setCursor(10, 100);
                sprite.print("WIPING PREFERENCES...");
                sprite.pushSprite(0, 0);
                preferences.clear();
                delay(1000);
                ESP.restart();
                return;
            }
            idx++; // EXPORT
            idx++; // IMPORT
        }
    }

    void executeDataMenuEdit(int dir, int& idx) {
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
    }

    void executeMenuEdit(int dir) {
        int idx = 1; // 0 is always <- Back

        if (activePage == PAGE_VISUALS) {
            executeVisualsMenuEdit(dir, idx);
        }
        else if (activePage == PAGE_ZONES) {
            executeZonesMenuEdit(dir, idx);
        }
        else if (activePage == PAGE_DEV) {
            executeDevMenuEdit(dir, idx);
        }
        else if (activePage == PAGE_DATA) {
            executeDataMenuEdit(dir, idx);
        }
    }
};

#endif
