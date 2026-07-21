#ifndef UIMANAGER_H
#define UIMANAGER_H
#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <RotaryEncoder.h>
#include <OneButton.h>
#include <Preferences.h>
#include "E54_Radar.h"
#include "ZoneManager.h"
#include "Themes.h"
#include "BroadcastServer.h"

enum AppState {
    STATE_BOOT,
    STATE_RADAR_VIEW,
    STATE_MENU,
    STATE_MENU_EDIT,
    STATE_GUIDE,
    STATE_IMPORTING,
    STATE_FALLBACK,
    STATE_CONFIRM_RESET,
    STATE_CONFIRM_WIFI_GEN,
    STATE_SELF_TEST,
    STATE_VIEW_WIFI
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
    THEME_MINIMAL,
    THEME_CYBERPUNK,
    THEME_TACTICAL
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


class UIManager;

class IView {
public:
    virtual ~IView() {}
    virtual void handleMenuClick(UIManager* ui) = 0;
    virtual void executeMenuEdit(UIManager* ui, int dir) = 0;
    virtual void populateMenuPage(UIManager* ui, char items[][32], int& numItems) = 0;
};


class MainMenuView : public IView {
public:
    void handleMenuClick(UIManager* ui) override;
    void executeMenuEdit(UIManager* ui, int dir) override;
    void populateMenuPage(UIManager* ui, char items[][32], int& numItems) override;
};

class VisualsMenuView : public IView {
public:
    void handleMenuClick(UIManager* ui) override;
    void executeMenuEdit(UIManager* ui, int dir) override;
    void populateMenuPage(UIManager* ui, char items[][32], int& numItems) override;
};

class ZonesMenuView : public IView {
public:
    void handleMenuClick(UIManager* ui) override;
    void executeMenuEdit(UIManager* ui, int dir) override;
    void populateMenuPage(UIManager* ui, char items[][32], int& numItems) override;
};

class DataMenuView : public IView {
public:
    void handleMenuClick(UIManager* ui) override;
    void executeMenuEdit(UIManager* ui, int dir) override;
    void populateMenuPage(UIManager* ui, char items[][32], int& numItems) override;
};

class DevMenuView : public IView {
public:
    void handleMenuClick(UIManager* ui) override;
    void executeMenuEdit(UIManager* ui, int dir) override;
    void populateMenuPage(UIManager* ui, char items[][32], int& numItems) override;
};

class UIManager {
public:
    IView* mainMenuView;
    IView* visualsMenuView;
    IView* zonesMenuView;
    IView* dataMenuView;
    IView* devMenuView;
    IView* activeView;

    ZoneManager zoneManager;
    Preferences preferences;
    String currentWifiPass = "";
    bool devRiskAccepted = false;
    bool motionCompEnabled = true;
    bool passthroughMode = true;
    bool broadcastModeEnabled = false;
    bool showStdDev = false;
    int uiTextSize = 1;
    float uiScale = 1.0f;

    UIManager(TFT_eSPI& display) : tft(display), sprite(&display), bgSprite(&display) {
        mainMenuView = new MainMenuView();
        visualsMenuView = new VisualsMenuView();
        zonesMenuView = new ZonesMenuView();
        dataMenuView = new DataMenuView();
        devMenuView = new DevMenuView();
        activeView = mainMenuView;

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
        sensitivity = 0;
        locationAveraging = 5;
        interpolationAmount = 0.5f;
        actionRequested = 0;
        currentMaxRangeMeters = 10;
        lastScaleExpandTime = 0;

        sweepAngle = 0;
        menuOverlayY = 0;
        maxMenuSelection = 0;
        bootStartTime = 0;

        for (int i=0; i<3; i++) {
            lastTargetActive[i] = false;
            targetCurrentX[i] = tft.width() / 2.0f;
            targetCurrentY[i] = 240.0f;
            lastDrawnX[i] = tft.width() / 2;
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
                targetHistoryX[i][h] = tft.width() / 2.0f;
                targetHistoryY[i][h] = 240.0f;
            }
        }
    }

    void updateThemeText() {
        if (theme == THEME_MINIMAL) themeText = 0xC618; // Light Grey
        else if (theme == THEME_ALIEN) themeText = 0x06DD; // themePrimary
        else themeText = TFT_WHITE;
    }

    void loadSettings() {
        preferences.begin("radar_ui", false);
        theme = (ThemeStyle)preferences.getInt("theme", THEME_ALIEN);
        updateThemeText();
        targetIcon = (TargetIcon)preferences.getInt("icon", ICON_SMART);
        sweepLineEnabled = preferences.getBool("sweep", true);
        trailLength = preferences.getInt("trails", 5);
        gridEnabled = preferences.getBool("grid", true);
        startupAnimEnabled = preferences.getBool("startup", true);
        simulatedSweep = preferences.getBool("simSwp", false);
        showStdDev = preferences.getBool("showStd", false);

        telemetryMode = (TelemetryMode)preferences.getInt("tData", TELEMETRY_OFF);
        uiTextSize = preferences.getInt("textSize", 1);
        uiScale = preferences.getFloat("uiScale", 1.0f);
        if (!preferences.getBool("sens_set", false)) {
            sensitivity = 0;
        } else {
            sensitivity = preferences.getInt("sens", 0);
        }
        locationAveraging = preferences.getInt("locAvg", 5);
        interpolationAmount = (float)preferences.getInt("interp", 5) * 0.1f;
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
        preferences.putBool("showStd", showStdDev);

        preferences.putInt("tData", telemetryMode);
        preferences.putInt("textSize", uiTextSize);
        preferences.putFloat("uiScale", uiScale);
        preferences.putBool("sens_set", true);
        preferences.putInt("sens", sensitivity);
        preferences.putInt("locAvg", locationAveraging);
        int interDisp = (int)(interpolationAmount * 10.0f + 0.5f);
        preferences.putInt("interp", interDisp);
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
        tft.fillScreen(themeBg); // Clear screen
        Serial.println("[UI] Screen filled with theme background");

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
        if (state == STATE_SELF_TEST) {
            state = STATE_MENU;
            return;
        }
        if (state == STATE_VIEW_WIFI) {
            state = STATE_MENU;
            return;
        }
        if (state == STATE_CONFIRM_RESET) {
            state = STATE_MENU;
            return;
        }
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
        } else if (state == STATE_CONFIRM_RESET || state == STATE_CONFIRM_WIFI_GEN) {
            if (dir != 0) {
                state = STATE_MENU;
            }
        }
    }


    void handleButtonLongPress() {
        if (state == STATE_MENU) {
            showTooltip = true;
        }
    }

    void handleButtonLongPressStop() {
        showTooltip = false;
    }

    void handleButton() {
        if (state == STATE_SELF_TEST) {
            state = STATE_MENU;
            return;
        }

        if (state == STATE_VIEW_WIFI) {
            state = STATE_MENU;
            return;
        }

        if (state == STATE_CONFIRM_RESET) {
            sprite.fillSprite(themeDanger);
            sprite.setTextColor(themeBg);
            int tw = sprite.textWidth("WIPING PREFERENCES...");
            sprite.setCursor((240 - tw) / 2, 100);
            sprite.print("WIPING PREFERENCES...");
            sprite.pushSprite(0, 0);
            preferences.clear();
            delay(1000);
            ESP.restart();
            return;
        } else if (state == STATE_CONFIRM_WIFI_GEN) {
            sprite.fillSprite(themeWarning);
            sprite.setTextColor(themeBg);
            sprite.setCursor(10, 100);
            sprite.print("REGENERATING WIFI PASSWORD...");
            sprite.pushSprite(0, 0);

            // To prevent circular include with ConfigManager.h inside UIManager.h
            // We just update the preferences here directly
            const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
            String newPass = "";
            for (int i = 0; i < 12; i++) {
                newPass += charset[esp_random() % (sizeof(charset) - 1)];
            }
            preferences.begin("radar_sys", false);
            preferences.putString("wifi_pass", newPass);
            preferences.end();

            sprite.fillSprite(themePrimary);
            sprite.setTextColor(themeBg);
            sprite.setCursor(10, 100);
            sprite.print("NEW PASS: ");
            sprite.print(newPass);
            sprite.pushSprite(0, 0);

            delay(3000);
            ESP.restart();
            return;
        } else if (state == STATE_IMPORTING) {
            actionRequested = 4; // Apply batched changes
            state = STATE_MENU;
        } else if (state == STATE_FALLBACK) {
            actionRequested = 3; // Confirm fallback
            state = STATE_RADAR_VIEW;
        } else if (state == STATE_CONFIRM_RESET) {
            sprite.fillSprite(themeDanger); // themeDanger
            sprite.setTextColor(themeBg);
            int tw = sprite.textWidth("WIPING PREFERENCES...");
            sprite.setCursor((240 - tw) / 2, 100);
            sprite.print("WIPING PREFERENCES...");
            sprite.pushSprite(0, 0);
            preferences.clear();
            delay(1000);
            ESP.restart();
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
            // Optimize: Replace costly division by 5000 with reciprocal multiplication (0.0002f).
            // Cast integer components to float to avoid implicit double-precision promotion overhead.
            targetVelX[index] = vx * (tft.width() / 2.0f) * 0.0002f;
            targetVelY[index] = -vy * (float)tft.width() * 0.0002f; // Y is inverted on screen
            targetAccX[index] = ax * (tft.width() / 2.0f) * 0.0002f;
            targetAccY[index] = -ay * (float)tft.width() * 0.0002f;
        }
    }

    // Call this when new radar data arrives (e.g. 10Hz) to set the goal targets

    void updateRadarData(RadarTarget targets[3], bool anchorValid, int16_t anchorX, int16_t anchorY) {
        this->anchorValid = anchorValid;
        this->anchorX = anchorX;
        this->anchorY = anchorY;

        // Calculate maximum distance of all active raw targets (in mm)
        float maxDistMM = 0.0f;
        for (int i = 0; i < 3; i++) {
            if (targets[i].active) {
                float dist = sqrtf((float)targets[i].x * targets[i].x + (float)targets[i].y * targets[i].y);
                if (dist > maxDistMM) {
                    maxDistMM = dist;
                }
            }
        }

        // Bound minimum range scale to 10m (10000mm), rounded up to 2m (2000mm) intervals
        int targetRangeMeters = 10;
        if (maxDistMM > 10000.0f) {
            int distM = (int)ceilf(maxDistMM / 1000.0f);
            if (distM % 2 != 0) distM += 1; // Round up to 2m interval
            targetRangeMeters = distM;
        }

        // Smooth range scaling with hysteresis (instantly expand when far target appears, hold 3s before shrinking)
        if (targetRangeMeters > currentMaxRangeMeters) {
            currentMaxRangeMeters = targetRangeMeters;
            lastScaleExpandTime = millis();
        } else if (targetRangeMeters < currentMaxRangeMeters) {
            if (millis() - lastScaleExpandTime > 3000) {
                currentMaxRangeMeters = targetRangeMeters;
            }
        }

        // Available vertical radius in pixels: 300px out of 320px
        float maxRangeMM = currentMaxRangeMeters * 1000.0f;
        float scalePxPerMm = 300.0f / maxRangeMM;

        for (int i = 0; i < 3; i++) {
            targetActive[i] = targets[i].active;
            targetCoasting[i] = targets[i].isCoasting;

            if (targets[i].active) {
                int16_t absSpeed = abs(targets[i].speed);
                if (sensitivity > 0 && absSpeed < sensitivity) {
                    targetActive[i] = false;
                } else {
                    targetGoalX[i] = (tft.width() / 2.0f) + (targets[i].x * scalePxPerMm) * uiScale;
                    targetGoalY[i] = tft.height() - (targets[i].y * scalePxPerMm) * uiScale;

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
        sprite.setTextColor(themeText, themeBg);
        sprite.setTextSize(uiTextSize);
        sprite.setCursor(10, 10);

        if (guidePage == 0) {
            sprite.print("GUIDE 1/3: TARGETS");
            sprite.setCursor(10, 40);
            sprite.setTextColor(themeText, themeBg);
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
            sprite.setTextColor(themeText, themeBg);
            sprite.print("Navigate via the dial:");

            sprite.setCursor(20, 80); sprite.print("- TURN: Scroll/Adjust");
            sprite.setCursor(20, 110); sprite.print("- PRESS: Select/Enter");
            sprite.setCursor(20, 140); sprite.print("- HOLD: Info Tooltips");



        } else if (guidePage == 2) {
            sprite.print("GUIDE 3/3: ZONES");
            sprite.setCursor(10, 40);
            sprite.setTextColor(themeText, themeBg);
            sprite.print("Zones highlight targets.");

            // Draw a mini radar zone
            sprite.drawCircle(tft.width() / 2, tft.height() / 2, 40, themeWarning);
            sprite.setCursor(10, 80); sprite.setTextColor(themeWarning, themeBg);
            sprite.print("Warning Zone (Amber)");

            sprite.drawCircle(tft.width() / 2, tft.height() / 2, 20, themeDanger);
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
        if (state == STATE_SELF_TEST) {
            drawSelfTestScreen();
            return;
        }
        if (state == STATE_GUIDE) {
            drawGuideScreen();
            return;
        }
        if (state == STATE_VIEW_WIFI) {
            drawWifiPassScreen();
            return;
        }

        advanceTargets();

        sprite.fillSprite(themeBg);

        if (theme == THEME_MINIMAL) {
            if (gridEnabled) {
                sprite.drawCircle(tft.width() / 2, tft.width(), 180, 0x18E3);
                sprite.fillCircle(tft.width() / 2, tft.width(), 4, 0x18E3);
            }
        } else {
            drawRadarBackground();
        }

        drawZones();
        drawSweepLine();

        animWarningPulse = (sinf(millis() * 0.0066667f) + 1.0f) * 0.5f;

        bool anyActive = false;
        for (int i = 0; i < 3; i++) {
            if (targetActive[i] && simAlpha[i] > 0.01f) {
                anyActive = true;
            }
            drawTarget(i);
        }

        if (!anyActive) {
            float pulse = (sinf(millis() * 0.00125f) + 1.0f) * 0.5f;
            uint16_t emptyColor = sprite.alphaBlend((uint8_t)(pulse * 150.0f) + 50, themePrimary, themeBg);
            sprite.setTextColor(emptyColor, themeBg);
            sprite.setTextSize(uiTextSize);
            int tw = sprite.textWidth("NO CONTACTS");
            sprite.setCursor((tft.width() - tw) / 2, 116);
            sprite.print("NO CONTACTS");

            sprite.setTextSize(1);
            int sw = sprite.textWidth("Waiting for movement...");
            sprite.setCursor((tft.width() - sw) / 2, 116 + (8 * uiTextSize) + 4);
            sprite.print("Waiting for movement...");
        }

        drawHUD();

        if (state == STATE_MENU || state == STATE_MENU_EDIT) {
            drawMenuOverlay();
        } else if (state == STATE_CONFIRM_WIFI_GEN) {
            float pulse = (sinf(millis() * 0.005f) + 1.0f) * 0.5f;
            uint16_t pulseColor = sprite.alphaBlend((uint8_t)(pulse * 100.0f) + 155, themeWarning, themeBg);
            sprite.fillRect(10, 90, 220, 60, pulseColor);
            sprite.setTextColor(themeBg, pulseColor);
            int w1 = sprite.textWidth("CONFIRM WIFI KEY REGEN");
            sprite.setCursor((240 - w1) / 2, 100);
            sprite.print("CONFIRM WIFI KEY REGEN");
            int w2 = sprite.textWidth("[PRESS] TO REGEN");
            sprite.setCursor((240 - w2) / 2, 115);
            sprite.print("[PRESS] TO REGEN");
            int w3 = sprite.textWidth("[TURN] TO CANCEL");
            sprite.setCursor((240 - w3) / 2, 130);
            sprite.print("[TURN] TO CANCEL");
        } else if (state == STATE_IMPORTING) {
            float pulse = (sinf(millis() * 0.005f) + 1.0f) * 0.5f;
            uint16_t pulseColor = sprite.alphaBlend((uint8_t)(pulse * 100.0f) + 155, themeWarning, themeBg);
            sprite.fillRect(10, 100, 220, 40, pulseColor);
            sprite.setTextColor(themeBg, pulseColor);

            int dots = (millis() / 500) % 4;
            const char* dotStr = (dots == 0) ? "" : (dots == 1) ? "." : (dots == 2) ? ".." : "...";
            char buf[32];
            snprintf(buf, sizeof(buf), "WAITING FOR CONFIG%-3s", dotStr);
            int w_dots = sprite.textWidth("WAITING FOR CONFIG...");
            sprite.setCursor((240 - w_dots) / 2, 110);
            sprite.print(buf);

            int w_apply = sprite.textWidth("[PRESS BUTTON TO APPLY]");
            sprite.setCursor((240 - w_apply) / 2, 125);
            sprite.print("[PRESS BUTTON TO APPLY]");
        } else if (state == STATE_FALLBACK) {
            float pulse = (sinf(millis() * 0.005f) + 1.0f) * 0.5f;
            uint16_t pulseColor = sprite.alphaBlend((uint8_t)(pulse * 100.0f) + 155, themeDanger, themeBg);
            sprite.fillRect(10, 90, 220, 60, pulseColor);
            sprite.setTextColor(themeBg, pulseColor);

            int w1 = sprite.textWidth("NEW CONFIG LOADED");
            sprite.setCursor((240 - w1) / 2, 100);
            sprite.print("NEW CONFIG LOADED");

            int w2 = sprite.textWidth("PRESS BUTTON TO KEEP");
            sprite.setCursor((240 - w2) / 2, 115);
            sprite.print("PRESS BUTTON TO KEEP");

            int dots = (millis() / 500) % 4;
            const char* dotStr = (dots == 0) ? "" : (dots == 1) ? "." : (dots == 2) ? ".." : "...";
            char buf[32];
            snprintf(buf, sizeof(buf), "OR WAIT TO REVERT%-3s", dotStr);
            int w3 = sprite.textWidth("OR WAIT TO REVERT...");
            sprite.setCursor((240 - w3) / 2, 130);
            sprite.print(buf);
        } else if (state == STATE_CONFIRM_RESET) {
            float pulse = (sinf(millis() * 0.005f) + 1.0f) * 0.5f;
            uint16_t pulseColor = sprite.alphaBlend((uint8_t)(pulse * 100.0f) + 155, themeDanger, themeBg);
            sprite.fillRect(10, 90, 220, 60, pulseColor);
            sprite.setTextColor(themeBg, pulseColor);

            int w1 = sprite.textWidth("CONFIRM FACTORY RESET");
            sprite.setCursor((240 - w1) / 2, 100);
            sprite.print("CONFIRM FACTORY RESET");

            int w2 = sprite.textWidth("[PRESS] TO WIPE");
            sprite.setCursor((240 - w2) / 2, 115);
            sprite.print("[PRESS] TO WIPE");

            int w3 = sprite.textWidth("[TURN] TO CANCEL");
            sprite.setCursor((240 - w3) / 2, 130);
            sprite.print("[TURN] TO CANCEL");
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

    uint32_t lastAdvanceMicros = 0;

    void advanceTargets() {
        uint32_t now = micros();
        float t = (lastAdvanceMicros == 0) ? 0.016f : (float)(now - lastAdvanceMicros) / 1000000.0f;
        if (t > 0.1f) t = 0.1f; // Clamp to 100ms max to prevent jumps on pause
        lastAdvanceMicros = now;

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
                    int targetDeg = (int)(targetRad * 57.2957795f);

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

            // Replace per-iteration sinf()/cosf() with fixed vector rotation.
            // Stepping by -2 degrees per iteration.
            // cos(-2 deg) ≈ 0.999390827f, sin(-2 deg) ≈ -0.034899496f
            constexpr float rotCos = 0.999390827f;
            constexpr float rotSin = -0.034899496f;

            float startRad = (sweepAngle - 180) * 0.0174533f;
            float dirX = cosf(startRad);
            float dirY = sinf(startRad);

            for (int a = 0; a < 30; a += 2) {
                int tx = (tft.width() / 2) + (180 * uiScale) * dirX;
                int ty = tft.height() + (180 * uiScale) * dirY;

                uint8_t alpha = 255 - ((a * 255) / 30);
                uint16_t trailCol = sprite.alphaBlend(alpha, sweepColor, themeBg);
                sprite.drawLine(120, 320, tx, ty, trailCol);

                float nextX = dirX * rotCos - dirY * rotSin;
                float nextY = dirY * rotCos + dirX * rotSin;
                dirX = nextX;
                dirY = nextY;
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
                    int tr = max(1, (int)((4 - (h / 2)) * uiScale));
                    sprite.fillCircle(hx, hy, tr, tColor);
                }
            }
        }
    }

    void drawTargetWarning(int i, int cx, int cy, uint8_t currentAlpha) {
        if (zoneManager.isWarning(i)) {
            float pulse = animWarningPulse;
            uint8_t blendRatio = (uint8_t)(pulse * 255.0f);
            uint16_t blendColor = sprite.alphaBlend(blendRatio, themeDanger, themeWarning);
            uint16_t wCol = sprite.alphaBlend(currentAlpha, blendColor, themeBg);
            int pr = (int)((8 + (pulse * 2.0f)) * uiScale);
            sprite.drawCircle(cx, cy, pr, wCol);
        }
        float danger = zoneManager.getTargetDangerLevel(i);
        if (danger > 0.01f) {
            uint16_t dangerColor = sprite.alphaBlend((uint8_t)(danger * 255.0f), themeDanger, themeWarning);
            uint16_t wCol = sprite.alphaBlend(currentAlpha, dangerColor, themeBg);

            float pulseSpeed = 300.0f - (danger * 200.0f);
            // Use single-precision sinf() to avoid implicit double conversion
            // inside 30Hz display rendering loop, saving CPU cycles on ESP32 FPU.
            float pulse = (sinf(millis() / pulseSpeed) + 1.0f) * 0.5f;
            int r = (int)((6 + (pulse * 4.0f * danger)) * uiScale);

            sprite.drawCircle(cx, cy, r, wCol);
        }
    }

    void drawTargetIcon(int i, int cx, int cy, uint16_t color) {
        if (targetIcon == ICON_CIRCLE) {
            int r1 = max(1, (int)(4 * uiScale));
            int r2 = max(1, (int)(5 * uiScale));
            sprite.fillCircle(cx, cy, r1, color);
            if (!targetCoasting[i]) sprite.drawCircle(cx, cy, r2, themePrimary);
        }
        else if (targetIcon == ICON_SQUARE) {
            int s1 = max(1, (int)(7 * uiScale));
            int s2 = max(1, (int)(9 * uiScale));
            sprite.fillRect(cx - s1/2, cy - s1/2, s1, s1, color);
            if (!targetCoasting[i]) sprite.drawRect(cx - s2/2, cy - s2/2, s2, s2, themePrimary);
        }
        else if (targetIcon == ICON_TRIANGLE) {
            sprite.fillTriangle(cx, cy - 5 * uiScale, cx - 4 * uiScale, cy + 3 * uiScale, cx + 4 * uiScale, cy + 3 * uiScale, color);
            if (!targetCoasting[i]) sprite.drawTriangle(cx, cy - 6 * uiScale, cx - 5 * uiScale, cy + 4 * uiScale, cx + 5 * uiScale, cy + 4 * uiScale, themePrimary);
        }
        else if (targetIcon == ICON_SMART) {
            sprite.drawCircle(cx, cy, max(1, (int)(3 * uiScale)), color);
            if (!targetCoasting[i]) sprite.drawCircle(cx, cy, max(1, (int)(4 * uiScale)), themePrimary);

            int absSpd = abs(rawTargetSpeed[i]);
            float rawDx = targetCurrentX[i] - targetHistoryX[i][2];
            float rawDy = targetCurrentY[i] - targetHistoryY[i][2];

            if (absSpd > 10 && (fabsf(rawDx) > 0.5f || fabsf(rawDy) > 0.5f)) {
                float len = sqrtf(rawDx*rawDx + rawDy*rawDy);
                float invLen = 1.0f / len;
                float nx = rawDx * invLen;
                float ny = rawDy * invLen;
                smoothVecX[i] = (smoothVecX[i] * 0.7f) + (nx * 0.3f);
                smoothVecY[i] = (smoothVecY[i] * 0.7f) + (ny * 0.3f);
                smoothSpeed[i] = (smoothSpeed[i] * 0.8f) + ((float)absSpd * 0.2f);
            } else {
                smoothSpeed[i] *= 0.8f;
            }

            if (smoothSpeed[i] > 5.0f) {
                float stickLen = (5.0f + (smoothSpeed[i] / 10.0f)) * uiScale;
                if (stickLen > 25.0f * uiScale) stickLen = 25.0f * uiScale;
                float sLen = sqrtf(smoothVecX[i]*smoothVecX[i] + smoothVecY[i]*smoothVecY[i]);
                if (sLen > 0.01f) {
                    float invSLen = 1.0f / sLen;
                    float nSvx = smoothVecX[i] * invSLen;
                    float nSvy = smoothVecY[i] * invSLen;
                    int ex = cx + (int)(nSvx * stickLen);
                    int ey = cy + (int)(nSvy * stickLen);
                    sprite.drawLine(cx, cy, ex, ey, color);

                    // Replace atan2f and cosf/sinf with fixed vector rotation.
                    // nSvx and nSvy are already the normalized direction vector.
                    // We can rotate this vector by +/- 0.5 radians using angle addition constants.
                    // cos(0.5) ≈ 0.87758256f, sin(0.5) ≈ 0.47942554f
                    constexpr float cos_05 = 0.87758256f;
                    constexpr float sin_05 = 0.47942554f;

                    // Rotate counter-clockwise (-0.5 radians)
                    int ax1 = ex - (int)(4.0f * uiScale * (nSvx * cos_05 + nSvy * sin_05));
                    int ay1 = ey - (int)(4.0f * uiScale * (nSvy * cos_05 - nSvx * sin_05));

                    // Rotate clockwise (+0.5 radians)
                    int ax2 = ex - (int)(4.0f * uiScale * (nSvx * cos_05 - nSvy * sin_05));
                    int ay2 = ey - (int)(4.0f * uiScale * (nSvy * cos_05 + nSvx * sin_05));

                    sprite.drawLine(ex, ey, ax1, ay1, color);
                    sprite.drawLine(ex, ey, ax2, ay2, color);
                }
            } else {
                sprite.fillCircle(cx, cy, max(1, (int)(2 * uiScale)), color);
            }
        }
    }

    void drawTarget(int i) {
        if (targetActive[i] && simAlpha[i] > 0.01f) {
            int cx = (int)targetCurrentX[i];
            int cy = (int)targetCurrentY[i];

            uint16_t baseColor;
            if (theme == THEME_MINIMAL) {
                baseColor = themePrimary;
            } else if (theme == THEME_ALIEN) {
                baseColor = themePrimary;
            } else {
                if (i == 0) baseColor = themeWarning;
                else if (i == 1) baseColor = themePrimary;
                else baseColor = themeDanger;
            }

            uint8_t currentAlpha = (uint8_t)(simAlpha[i] * 255.0f);
            uint16_t color = sprite.alphaBlend(currentAlpha, baseColor, themeBg);

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

            // Draw StdDev Visualization Circle
            if (showStdDev && targetStdDev[i] > 1.0f) {
                // Convert millimeter stdDev to screen pixels (approx 120 pixels per 5000 mm)
                float screenRadius = targetStdDev[i] * (120.0f * 0.0002f) * uiScale;
                if (screenRadius < 2.0f * uiScale) screenRadius = 2.0f * uiScale;
                if (screenRadius > 120.0f) screenRadius = tft.width() / 2.0f;
                uint16_t devColor = sprite.alphaBlend(100, baseColor, themeBg); // subtle wireframe
                sprite.drawCircle(cx, cy, (int)screenRadius, devColor);
            }

            if (zoneManager.isWarning(i)) {
                float pulse = animWarningPulse;
                uint8_t blendRatio = (uint8_t)(pulse * 255.0f);
                uint16_t blendColor = sprite.alphaBlend(blendRatio, themeDanger, themeWarning);
                uint16_t wCol = sprite.alphaBlend(currentAlpha, blendColor, themeBg);
                int pr = (int)((8 + (pulse * 2.0f)) * uiScale);
                sprite.drawCircle(cx, cy, pr, wCol);
            }
            float danger = zoneManager.getTargetDangerLevel(i);
            if (danger > 0.01f) {
                uint16_t dangerColor = sprite.alphaBlend((uint8_t)(danger * 255.0f), themeDanger, themeWarning);
                uint16_t wCol = sprite.alphaBlend(currentAlpha, dangerColor, themeBg);

                float pulseSpeed = 300.0f - (danger * 200.0f);
                // Use single-precision sinf() to avoid implicit double conversion
                // inside 30Hz display rendering loop, saving CPU cycles on ESP32 FPU.
                float pulse = (sinf(millis() / pulseSpeed) + 1.0f) * 0.5f;
                int r = (int)((6 + (pulse * 4.0f * danger)) * uiScale);

                sprite.drawCircle(cx, cy, r, wCol);
            }

            if (targetIcon == ICON_CIRCLE) {
                int r1 = max(1, (int)(4 * uiScale));
                int r2 = max(1, (int)(5 * uiScale));
                sprite.fillCircle(cx, cy, r1, color);
                if (!targetCoasting[i]) sprite.drawCircle(cx, cy, r2, themePrimary);
            }
            else if (targetIcon == ICON_SQUARE) {
                int s1 = max(1, (int)(7 * uiScale));
                int s2 = max(1, (int)(9 * uiScale));
                sprite.fillRect(cx - s1/2, cy - s1/2, s1, s1, color);
                if (!targetCoasting[i]) sprite.drawRect(cx - s2/2, cy - s2/2, s2, s2, themePrimary);
            }
            else if (targetIcon == ICON_TRIANGLE) {
                sprite.fillTriangle(cx, cy - 5 * uiScale, cx - 4 * uiScale, cy + 3 * uiScale, cx + 4 * uiScale, cy + 3 * uiScale, color);
                if (!targetCoasting[i]) sprite.drawTriangle(cx, cy - 6 * uiScale, cx - 5 * uiScale, cy + 4 * uiScale, cx + 5 * uiScale, cy + 4 * uiScale, themePrimary);
            }
            else if (targetIcon == ICON_SMART) {
                sprite.drawCircle(cx, cy, max(1, (int)(3 * uiScale)), color);
                if (!targetCoasting[i]) sprite.drawCircle(cx, cy, max(1, (int)(4 * uiScale)), themePrimary);

                int absSpd = abs(rawTargetSpeed[i]);
                float rawDx = targetCurrentX[i] - targetHistoryX[i][2];
                float rawDy = targetCurrentY[i] - targetHistoryY[i][2];

                if (absSpd > 10 && (fabsf(rawDx) > 0.5f || fabsf(rawDy) > 0.5f)) {
                    float len = sqrtf(rawDx*rawDx + rawDy*rawDy);
                    float invLen = 1.0f / len;
                    float nx = rawDx * invLen;
                    float ny = rawDy * invLen;
                    smoothVecX[i] = (smoothVecX[i] * 0.7f) + (nx * 0.3f);
                    smoothVecY[i] = (smoothVecY[i] * 0.7f) + (ny * 0.3f);
                    smoothSpeed[i] = (smoothSpeed[i] * 0.8f) + ((float)absSpd * 0.2f);
                } else {
                    smoothSpeed[i] *= 0.8f;
                }

                if (smoothSpeed[i] > 5.0f) {
                    float stickLen = 5.0f + (smoothSpeed[i] * 0.1f);
                    if (stickLen > 25.0f) stickLen = 25.0f;
                    float sLen = sqrtf(smoothVecX[i]*smoothVecX[i] + smoothVecY[i]*smoothVecY[i]);
                    if (sLen > 0.01f) {
                        float invSLen = 1.0f / sLen;
                        float nSvx = smoothVecX[i] * invSLen;
                        float nSvy = smoothVecY[i] * invSLen;
                        int ex = cx + (int)(nSvx * stickLen);
                        int ey = cy + (int)(nSvy * stickLen);
                        sprite.drawLine(cx, cy, ex, ey, color);

                        // Replace atan2f and cosf/sinf with fixed vector rotation.
                        // nSvx and nSvy are already the normalized direction vector.
                        // We can rotate this vector by +/- 0.5 radians using angle addition constants.
                        // cos(0.5) ≈ 0.87758256f, sin(0.5) ≈ 0.47942554f
                        constexpr float cos_05 = 0.87758256f;
                        constexpr float sin_05 = 0.47942554f;

                        // Rotate counter-clockwise (-0.5 radians)
                        int ax1 = ex - (int)(4.0f * (nSvx * cos_05 + nSvy * sin_05));
                        int ay1 = ey - (int)(4.0f * (nSvy * cos_05 - nSvx * sin_05));

                        // Rotate clockwise (+0.5 radians)
                        int ax2 = ex - (int)(4.0f * (nSvx * cos_05 - nSvy * sin_05));
                        int ay2 = ey - (int)(4.0f * (nSvy * cos_05 + nSvx * sin_05));

                        sprite.drawLine(ex, ey, ax1, ay1, color);
                        sprite.drawLine(ex, ey, ax2, ay2, color);
                    }
                } else {
                    sprite.fillCircle(cx, cy, 2, color);
                }
            }

            if (theme != THEME_ALIEN && telemetryMode != TELEMETRY_OFF) {
                sprite.setTextColor(color, themeBg);

                sprite.setCursor(cx + 8, cy - 12);

                if (telemetryMode == TELEMETRY_DIST_ANG) {
                    float dist_m = sqrtf((long)rawTargetX[i]*rawTargetX[i] + (long)rawTargetY[i]*rawTargetY[i]) * 0.001f;
                    int angle = (int)(atan2f((float)rawTargetX[i], (float)rawTargetY[i]) * 57.2957795f);
                    sprite.printf("%.1fm %ddeg", dist_m, angle);
                } else if (telemetryMode == TELEMETRY_VELOCITY) {
                    float speed_ms = (float)rawTargetSpeed[i] * 0.1f;
                    sprite.printf("%.1fm/s", speed_ms);
                } else if (telemetryMode == TELEMETRY_RAW) {
                    sprite.printf("%dmm,%dmm", rawTargetX[i], rawTargetY[i]);
                } else if (telemetryMode == TELEMETRY_ALL) {
                    float dist_m = sqrtf((long)rawTargetX[i]*rawTargetX[i] + (long)rawTargetY[i]*rawTargetY[i]) * 0.001f;
                    int angle = (int)(atan2f((float)rawTargetX[i], (float)rawTargetY[i]) * 57.2957795f);
                    float speed_ms = (float)rawTargetSpeed[i] * 0.1f;
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

        sprite.setTextColor(themeText, themeBg);
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
                sprite.setTextColor(themeText, themeBg);
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
public:
    TFT_eSprite sprite;
    TFT_eSprite bgSprite;
    bool bgCacheValid = false;
    int lastBgTheme = -1;
    bool lastBgGridEnabled = false;
    int lastBgMaxRangeMeters = -1;
    float lastBgUiScale = -1.0f;
public:
public:
    AppState state;
    MenuPage activePage;
    int menuSelection;
    int menuOverlayY;
    int maxMenuSelection;
    char currentMenuItems[24][32];
    int currentMenuNumItems = 0;
    int guidePage = 0;
    bool showTooltip = false;
    unsigned long bootStartTime;

    ThemeStyle theme;
    uint16_t themeText;
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
    int currentMaxRangeMeters = 10;
    uint32_t lastScaleExpandTime = 0;

    int sweepAngle;
    int actionRequested;
    float animWarningPulse = 0.0f;
    bool selfTestDone = false;
    bool selfTestRxOk = false;
    bool selfTestSoftwareOk = false;

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

    void drawWifiPassScreen() {
        sprite.fillSprite(themeBg);

        // Draw Header
        sprite.fillRect(0, 0, 240, 30, themePrimary);
        sprite.setTextColor(themeBg, themePrimary);
        sprite.setTextSize(uiTextSize);
        int titleTw = sprite.textWidth("WIFI AP KEY");
        sprite.setCursor((tft.width() - titleTw) / 2, 8);
        sprite.print("WIFI AP KEY");

        // Draw Subtext
        sprite.setTextColor(sprite.alphaBlend(150, themePrimary, themeBg), themeBg);
        int subTw = sprite.textWidth("Current Broadcast Key");
        sprite.setCursor((tft.width() - subTw) / 2, 60);
        sprite.print("Current Broadcast Key");

        // Draw Key Frame
        int boxW = 200;
        int boxH = 50;
        int boxX = (tft.width() - boxW) / 2;
        int boxY = 90;
        sprite.fillRoundRect(boxX, boxY, boxW, boxH, 8, sprite.alphaBlend(40, themePrimary, themeBg));
        sprite.drawRoundRect(boxX, boxY, boxW, boxH, 8, themePrimary);

        // Draw Key String
        sprite.setTextColor(themeSuccess, sprite.alphaBlend(40, themePrimary, themeBg));
        sprite.setTextSize(2);
        int keyTw = sprite.textWidth(currentWifiPass);
        sprite.setCursor((tft.width() - keyTw) / 2, boxY + 18);
        sprite.print(currentWifiPass);
        sprite.setTextSize(uiTextSize);

        // Draw Footer
        sprite.setTextColor(themeWarning, themeBg);
        int footTw = sprite.textWidth("Click or Turn to Exit");
        sprite.setCursor((tft.width() - footTw) / 2, 280);
        sprite.print("Click or Turn to Exit");

        sprite.pushSprite(0, 0);
    }

    void drawSelfTestScreen() {
        sprite.fillSprite(themeBg);
        sprite.setTextColor(themeText, themeBg);
        sprite.setTextSize(uiTextSize);
        sprite.setCursor(10, 10);
        sprite.print("--- SELF TEST ---");

        sprite.setCursor(10, 40);
        if (!selfTestDone) {
            float pulse = (sinf(millis() * 0.005f) + 1.0f) * 0.5f;
            uint16_t pulseColor = sprite.alphaBlend((uint8_t)(pulse * 100.0f) + 155, activeTheme.warning, activeTheme.bg);
            sprite.fillRect(10, 40, 220, 40, pulseColor);
            sprite.setTextColor(activeTheme.bg, pulseColor);
            sprite.setCursor(20, 55);

            int dots = (millis() / 500) % 4;
            const char* dotStr = (dots == 0) ? "" : (dots == 1) ? "." : (dots == 2) ? ".." : "...";
            sprite.printf("RUNNING TESTS%-3s", dotStr);
        } else {
            sprite.print("Wiring / RX Check:");
            sprite.setCursor(10, 60);
            if (selfTestRxOk) {
                sprite.setTextColor(themeSuccess, themeBg);
                sprite.print("PASS: RX is HIGH");
            } else {
                sprite.setTextColor(themeDanger, themeBg);
                sprite.print("FAIL: RX is LOW");
            }

            sprite.setTextColor(themeText, themeBg);
            sprite.setCursor(10, 80);
            sprite.print("Software Logic Check:");
            sprite.setCursor(10, 100);
            if (selfTestSoftwareOk) {
                sprite.setTextColor(themeSuccess, themeBg);
                sprite.print("PASS: Tests passed");
            } else {
                sprite.setTextColor(themeDanger, themeBg);
                sprite.print("FAIL: Tests failed");
            }
        }

        sprite.setTextColor(themeWarning, themeBg);
        sprite.setCursor(10, 140);
        sprite.print("Click to exit");

        sprite.pushSprite(0, 0);
    }

    void drawBootScreen() {
        sprite.fillSprite(themeBg);
        unsigned long elapsed = millis() - bootStartTime;

        int maxR = (elapsed * 180) / 1000;
        if (maxR > 180) maxR = 180;

        uint16_t gridColor = (theme == THEME_ALIEN) ? themePrimary : themePrimary;

        // Hoist trigonometry out of radial rendering loops
        for (int a = -180; a <= 180; a += 5) {
            float rad = a * 0.0174533f;
            float cosA = cosf(rad);
            float sinA = sinf(rad);
            for (int r = 60; r <= 180; r += 60) {
                if (maxR >= r) {
                    int sweepDeg = ((maxR - r) * 180) / 30;
                    if (sweepDeg > 360) sweepDeg = 360;
                    if (a < -180 + sweepDeg) {
                        sprite.drawPixel((tft.width() / 2) + r * cosA, tft.width() + r * sinA, gridColor);
                    }
                }
            }
        }

        if (maxR > 0) {
            sprite.drawLine(tft.width() / 2, tft.width(), tft.width() / 2, tft.width() - maxR, gridColor);
            sprite.drawLine(tft.width() / 2, tft.width(), (tft.width() / 2) - maxR, tft.width(), gridColor);
            sprite.drawLine(tft.width() / 2, tft.width(), (tft.width() / 2) + maxR, tft.width(), gridColor);
        }

        sprite.setTextColor(themeText, themeBg);
        sprite.setTextSize(uiTextSize);

        const char* statusText = "";
        if (elapsed < 300) statusText = "INIT";
        else if (elapsed < 600) statusText = "CALIBRATING";
        else if (elapsed < 1000) statusText = "SCANNING...";

        if (statusText[0] != '\0') {
            int textW = sprite.textWidth(statusText);
            sprite.setCursor((tft.width() - textW) / 2, 120);
            sprite.print(statusText);
        }

        tft.startWrite(); sprite.pushSprite(0, 0); tft.endWrite();  // PSRAM-safe push, full 240x320

        if (elapsed > 1200) {
            state = STATE_RADAR_VIEW;
        }
    }

    void drawRadialWedge(int minDist, int maxDist, int minAngle, int maxAngle, uint16_t color) {
        int maxR = ((maxDist * 180) / 6000) * uiScale;
        int minR = ((minDist * 180) / 6000) * uiScale;
        if (maxR > 180) maxR = 180;
        if (minR > 180) minR = 180;

        // Replace per-iteration sinf()/cosf() with fixed vector rotation
        // for iterative arcs, avoiding expensive FPU trigonometric evaluations.
        // 1 degree in radians = 0.0174533f
        // cos(1 degree) ≈ 0.999847695156f
        // sin(1 degree) ≈ 0.017452406437f
        constexpr float rotCos = 0.999847695156f;
        constexpr float rotSin = 0.017452406437f;

        float startRad = (minAngle - 90) * 0.0174533f;
        float dirX = cosf(startRad);
        float dirY = sinf(startRad);

        for (int a = minAngle; a <= maxAngle; a++) {
            sprite.drawLine((tft.width() / 2) + minR*dirX, tft.width() + minR*dirY, (tft.width() / 2) + maxR*dirX, tft.width() + maxR*dirY, color);

            float nextX = dirX * rotCos - dirY * rotSin;
            float nextY = dirY * rotCos + dirX * rotSin;
            dirX = nextX;
            dirY = nextY;
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
        if (!gridEnabled) return;

        // Check if cached background is still valid
        if (bgCacheValid &&
            lastBgTheme == (int)theme &&
            lastBgGridEnabled == gridEnabled &&
            lastBgMaxRangeMeters == currentMaxRangeMeters &&
            lastBgUiScale == uiScale) {
            bgSprite.pushToSprite(&sprite, 0, 0);
            return;
        }

        // Re-render static background grid into bgSprite buffer
        bgSprite.fillSprite(themeBg);

        int originX = tft.width() / 2;     // 120
        int originY = tft.height();         // 320
        float maxRangeMM = currentMaxRangeMeters * 1000.0f;
        float scalePxPerMm = 300.0f / maxRangeMM;

        uint16_t primaryColor = (theme == THEME_ALIEN) ? themePrimary : TFT_GREEN;
        uint16_t heavyGridColor = bgSprite.alphaBlend(150, primaryColor, themeBg); // Heavy stroke (even meters)
        uint16_t lightGridColor = bgSprite.alphaBlend(65, primaryColor, themeBg);  // Light stroke (odd meters)

        // 1. Concentric Polar Distance Arcs (1m increments up to currentMaxRangeMeters)
        for (int m = 1; m <= currentMaxRangeMeters; m++) {
            int r_px = (int)(m * 1000.0f * scalePxPerMm * uiScale);
            if (r_px <= 0 || r_px > originY + 50) continue;

            bool isEven = (m % 2 == 0);
            uint16_t arcColor = isEven ? heavyGridColor : lightGridColor;

            if (isEven) {
                // Heavy stroke (2-pixel thick concentric arc)
                bgSprite.drawCircle(originX, originY, r_px, arcColor);
                if (r_px > 1) bgSprite.drawCircle(originX, originY, r_px - 1, arcColor);

                // Distance label on even meters along center vertical axis
                bgSprite.setTextColor(bgSprite.alphaBlend(200, primaryColor, themeBg), themeBg);
                bgSprite.setTextSize(1);
                bgSprite.setCursor(originX + 4, originY - r_px - 4);
                bgSprite.printf("%dm", m);
            } else {
                // Light stroke (1-pixel concentric arc)
                bgSprite.drawCircle(originX, originY, r_px, arcColor);
            }
        }

        // 2. Polar Radial Spoke Rays (-60°, -45°, -30°, 0°, 30°, 45°, 60°)
        int maxR = (int)(currentMaxRangeMeters * 1000.0f * scalePxPerMm * uiScale);

        // Heavy vertical centerline (0° / 90° straight up)
        bgSprite.drawLine(originX, originY, originX, originY - maxR, heavyGridColor);

        int spokeAngles[] = {-60, -45, -30, 30, 45, 60};
        for (int deg : spokeAngles) {
            float rad = (deg - 90) * 0.0174532925f;
            int rx = originX + (int)(maxR * cosf(rad));
            int ry = originY + (int)(maxR * sinf(rad));

            bool isHeavy = (deg == -45 || deg == 45);
            uint16_t rayColor = isHeavy ? heavyGridColor : lightGridColor;
            bgSprite.drawLine(originX, originY, rx, ry, rayColor);
        }

        // Heavy baseline across bottom
        bgSprite.drawLine(0, originY - 1, tft.width(), originY - 1, heavyGridColor);

        // Mark background cache as valid
        lastBgTheme = (int)theme;
        lastBgGridEnabled = gridEnabled;
        lastBgMaxRangeMeters = currentMaxRangeMeters;
        lastBgUiScale = uiScale;
        bgCacheValid = true;

        bgSprite.pushToSprite(&sprite, 0, 0);
    }


    void populateMainMenu(char items[][32], int& numItems) {
        sprite.setTextColor(themeText, themeBg);
        sprite.setCursor(15, 5); sprite.print("CONFIG MENU");

        snprintf(items[numItems++], 32, "%s", "VISUAL SETTINGS");
        snprintf(items[numItems++], 32, "%s", "  [DISPLAY/HUD]");
        snprintf(items[numItems++], 32, "%s", "ZONE CONFIG");
        snprintf(items[numItems++], 32, "%s", "  [BOUNDARIES]");
        snprintf(items[numItems++], 32, "%s", "TARGET DATA");
        snprintf(items[numItems++], 32, "%s", "  [GAIN/FILTER]");
        snprintf(items[numItems++], 32, "%s", "DEV OPTIONS");
        snprintf(items[numItems++], 32, "%s", "USER GUIDE");
        snprintf(items[numItems++], 32, "%s", "[ Exit Menu ]");
    }

    void populateVisualsMenu(char items[][32], int& numItems) {
        sprite.setTextColor(themeText, themeBg);
        sprite.setCursor(15, 5); sprite.print("--- VISUAL SETTINGS ---");

        const char* themeStr = (theme == THEME_STANDARD) ? "Standard" :
                         (theme == THEME_ALIEN) ? "Alien" :
                         (theme == THEME_MINIMAL) ? "Minimal" :
                         (theme == THEME_CYBERPUNK) ? "Cyberpunk" : "Tactical";
        const char* iconStr = (targetIcon == ICON_CIRCLE) ? "CIRCLE" :
                         (targetIcon == ICON_SQUARE) ? "SQUARE" :
                         (targetIcon == ICON_TRIANGLE) ? "TRIANGLE" : "SMART";
        snprintf(items[numItems++], 32, "%s", "<- Back");
        snprintf(items[numItems++], 32, "Theme: %s", themeStr);
        snprintf(items[numItems++], 32, "Icon: %s", iconStr);
        snprintf(items[numItems++], 32, "Text Size: %d", uiTextSize);
        snprintf(items[numItems++], 32, "UI Scale: %.1f", uiScale);
        snprintf(items[numItems++], 32, "Sweep Line: %s", sweepLineEnabled ? "ON" : "OFF");
        snprintf(items[numItems++], 32, "Sweep Mode: %s", simulatedSweep ? "SIMULATED" : "VISUAL");
        snprintf(items[numItems++], 32, "Trails: %d frames", trailLength);
        snprintf(items[numItems++], 32, "Grid: %s", gridEnabled ? "ON" : "OFF");
        snprintf(items[numItems++], 32, "Boot Anim: %s", startupAnimEnabled ? "ON" : "OFF");
    }

    void populateZonesMenu(char items[][32], int& numItems) {
        sprite.setTextColor(themeText, themeBg);
        sprite.setCursor(15, 5); sprite.print("--- ZONE CONFIG ---");

        const char* warnStr = (zoneManager.getWarnPreset() == ZONE_OFF) ? "OFF" :
                         (zoneManager.getWarnPreset() == ZONE_CLOSE) ? "CLOSE" :
                         (zoneManager.getWarnPreset() == ZONE_MEDIUM) ? "MED" :
                         (zoneManager.getWarnPreset() == ZONE_FAR) ? "FAR" : "CUSTOM";

        const char* deadStr = (zoneManager.getDeadPreset() == ZONE_OFF) ? "OFF" :
                         (zoneManager.getDeadPreset() == ZONE_CLOSE) ? "CLOSE" :
                         (zoneManager.getDeadPreset() == ZONE_MEDIUM) ? "MED" :
                         (zoneManager.getDeadPreset() == ZONE_FAR) ? "FAR" : "CUSTOM";

        snprintf(items[numItems++], 32, "%s", "<- Back");
        snprintf(items[numItems++], 32, "Warn Zone: %s", warnStr);
        if (zoneManager.getWarnPreset() == ZONE_CUSTOM) {
            snprintf(items[numItems++], 32, " W-MinD: %dmm", zoneManager.getWarnCustom().minDist);
            snprintf(items[numItems++], 32, " W-MaxD: %dmm", zoneManager.getWarnCustom().maxDist);
            snprintf(items[numItems++], 32, " W-MinA: %ddeg", zoneManager.getWarnCustom().minAngle);
            snprintf(items[numItems++], 32, " W-MaxA: %ddeg", zoneManager.getWarnCustom().maxAngle);
        }
        if (zoneManager.getWarnPreset() != ZONE_OFF) {
            snprintf(items[numItems++], 32, "Warn Fuzz: %d%%", zoneManager.getFuzzingThreshold());
            snprintf(items[numItems++], 32, "Warn Time: %dms", zoneManager.getHistoryWindow() * 100);
        }

        snprintf(items[numItems++], 32, "Dead Zone: %s", deadStr);
        if (zoneManager.getDeadPreset() == ZONE_CUSTOM) {
            snprintf(items[numItems++], 32, " D-MinD: %dmm", zoneManager.getDeadCustom().minDist);
            snprintf(items[numItems++], 32, " D-MaxD: %dmm", zoneManager.getDeadCustom().maxDist);
            snprintf(items[numItems++], 32, " D-MinA: %ddeg", zoneManager.getDeadCustom().minAngle);
            snprintf(items[numItems++], 32, " D-MaxA: %ddeg", zoneManager.getDeadCustom().maxAngle);
        }
    }

    void populateDataMenu(char items[][32], int& numItems) {
        sprite.setTextColor(themeText, themeBg);
        sprite.setCursor(15, 5); sprite.print("--- TARGET DATA ---");

        const char* tDataStr = (telemetryMode == TELEMETRY_OFF) ? "OFF" :
                          (telemetryMode == TELEMETRY_DIST_ANG) ? "DIST/ANG" :
                          (telemetryMode == TELEMETRY_VELOCITY) ? "SPEED" :
                          (telemetryMode == TELEMETRY_RAW) ? "RAW X/Y" : "ALL";
        int interDisp = (int)(interpolationAmount * 10.0f + 0.5f);

        snprintf(items[numItems++], 32, "%s", "<- Back");
        snprintf(items[numItems++], 32, "Telemetry: %s", tDataStr);
        snprintf(items[numItems++], 32, "Sensitivity: %d cm/s", sensitivity);
        snprintf(items[numItems++], 32, "Loc Avg: %d frames", locationAveraging);
        snprintf(items[numItems++], 32, "Smoothing: %d", interDisp);
        snprintf(items[numItems++], 32, "%s", "[ Reset Tracking ]");
    }

    void populateDevMenu(char items[][32], int& numItems) {
        sprite.setTextColor(themeDanger, themeBg);
        sprite.setCursor(15, 5); sprite.print("--- DEV OPTIONS ---");

        snprintf(items[numItems++], 32, "%s", "<- Back");
        snprintf(items[numItems++], 32, "Accept Risk? %s", devRiskAccepted ? "YES" : "NO");
        if (devRiskAccepted) {
            snprintf(items[numItems++], 32, "Motion Comp: %s", motionCompEnabled ? "ON" : "OFF");
            snprintf(items[numItems++], 32, "Passthrough: %s", passthroughMode ? "ON" : "OFF");
            snprintf(items[numItems++], 32, "Broadcast AP: %s", broadcastModeEnabled ? "ON" : "OFF");
            snprintf(items[numItems++], 32, "Show StdDev: %s", showStdDev ? "ON" : "OFF");
            snprintf(items[numItems++], 32, "%s", "[ RUN SELF TEST ]");
            snprintf(items[numItems++], 32, "%s", "[ FACTORY RESET ]");
            snprintf(items[numItems++], 32, "%s", "[ EXPORT CONFIG ]");
            snprintf(items[numItems++], 32, "%s", "[ IMPORT CONFIG ]");
            snprintf(items[numItems++], 32, "%s", "[ VIEW WIFI PASS ]");
            snprintf(items[numItems++], 32, "%s", "[ REGEN WIFI PASS ]");
        }
    }

    void populateMenuPage(char items[][32], int& numItems) {
        if (activeView) activeView->populateMenuPage(this, items, numItems);
    }
    void drawMenuScrollbar(int numItems, int startIdx) {
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

        struct TooltipMapping {
        const char* prefix;
        const char* text;
    };

    static constexpr TooltipMapping tooltips[] = {
        {"<- Back", "Return to previous menu."},
        {"VISUAL SETTINGS", "Colors, icons, & layout."},
        {"  [DISPLAY/HUD]", "Colors, icons, & layout."},
        {"ZONE CONFIG", "Warning & dead zones."},
        {"  [BOUNDARIES]", "Warning & dead zones."},
        {"TARGET DATA", "Data processing & limits."},
        {"  [GAIN/FILTER]", "Data processing & limits."},
        {"DEV OPTIONS", "Advanced & experimental."},
        {"USER GUIDE", "Help & instructions."},
        {"[ Exit Menu ]", "Return to radar view."},
        {"Theme:", "Change color palette."},
        {"Icon:", "Change target marker."},
        {"Text Size:", "UI text scale (1-2)."},
        {"UI Scale:", "UI element scale."},
        {"Sweep Line:", "Toggle scanning line."},
        {"Sweep Mode:", "Simulated vs physical."},
        {"Trails:", "Target history length."},
        {"Grid:", "Toggle background grid."},
        {"Boot Anim:", "Toggle startup sequence."},
        {"Warn Zone:", "Visual alert area."},
        {" W-MinD:", "Minimum distance (mm)."},
        {" D-MinD:", "Minimum distance (mm)."},
        {" W-MaxD:", "Maximum distance (mm)."},
        {" D-MaxD:", "Maximum distance (mm)."},
        {" W-MinA:", "Left-most angle (deg)."},
        {" D-MinA:", "Left-most angle (deg)."},
        {" W-MaxA:", "Right-most angle (deg)."},
        {" D-MaxA:", "Right-most angle (deg)."},
        {"Warn Fuzz:", "Boundary tolerance (%)."},
        {"Warn Time:", "Time to trigger alert."},
        {"Dead Zone:", "Ignore targets area."},
        {"Telemetry:", "On-screen target data."},
        {"Sensitivity:", "Min target speed (cm/s)."},
        {"Loc Avg:", "Position smoothing frames."},
        {"Smoothing:", "Movement interpolation."},
        {"[ Reset Tracking ]", "Clear all targets."},
        {"Accept Risk?", "Enable advanced features?"},
        {"Motion Comp:", "Compensate for host movement."},
        {"Passthrough:", "Raw UART to serial."},
        {"Broadcast AP:", "Host a local Wi-Fi network."},
        {"Show StdDev:", "Display data variance."},
        {"[ RUN SELF TEST ]", "Execute diagnostics."},
        {"[ VIEW WIFI PASS ]", "Display current AP key."},
        {"[ REGEN WIFI PASS ]", "Generate new AP key."},
        {"[ FACTORY RESET ]", "Erase all settings."},
        {"[ EXPORT CONFIG ]", "Save settings to SD."},
        {"[ IMPORT CONFIG ]", "Load settings from SD."},
        {"Zoom:", "Visual radar zoom level."},
    };

    void drawMenuTooltip(const char* selectedItemText) {
        sprite.fillRect(10, 140, 220, 60, themeBg);
        sprite.drawRect(10, 140, 220, 60, themeWarning);
        sprite.setTextColor(themeText, themeBg);
        sprite.setTextSize(uiTextSize);
        sprite.setCursor(15, 145);
        sprite.print("INFO: ");
        sprite.setCursor(15, 160);

        String selItem = String(selectedItemText);
        const char* tooltipText = "Adjust setting value."; // Default fallback

        for (const auto& mapping : tooltips) {
            if (selItem.startsWith(mapping.prefix)) {
                tooltipText = mapping.text;
                break;
            }
        }

        sprite.print(tooltipText);
    }
    void drawMenuItems(char items[][32], int numItems) {
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
                sprite.setTextColor(themeText, themeBg);
            }

            int maxWidth = 210;
            int tWidth = sprite.textWidth(items[idx]);

            if (tWidth > maxWidth) {
                int overflow = tWidth - maxWidth;
                int scrollSpeed = 50;
                int cycleTime = (overflow * scrollSpeed) + 2000;
                int t = millis() % cycleTime;

                int scrollX = 0;
                if (t > 1000) {
                    scrollX = (t - 1000) / scrollSpeed;
                    if (scrollX > overflow) scrollX = overflow;
                }

                sprite.setViewport(15, yPos, maxWidth, 24);
                sprite.setCursor(-scrollX, 0);
                sprite.print(items[idx]);
                sprite.resetViewport();
            } else {
                sprite.setCursor(15, yPos);
                sprite.print(items[idx]);
            }
        }

        if (numItems > 4) {
            drawMenuScrollbar(numItems, startIdx);
        }

        if (showTooltip) {
            drawMenuTooltip(items[menuSelection]);
        }
    }

    void drawMenuOverlay() {
        if (menuOverlayY < 200) menuOverlayY += 15;

        sprite.fillRect(0, 0, 240, menuOverlayY, sprite.alphaBlend(220, themeBg, themePrimary));
        sprite.drawLine(0, menuOverlayY, 240, menuOverlayY, themePrimary);
        if (menuOverlayY < 200) return;

        sprite.setTextSize(uiTextSize);
        currentMenuNumItems = 0;

        populateMenuPage(currentMenuItems, currentMenuNumItems);
        drawMenuItems(currentMenuItems, currentMenuNumItems);
    }

    void handleMenuClick() {
        if (activeView) activeView->handleMenuClick(this);
        if (activePage == PAGE_MAIN) activeView = mainMenuView;
        else if (activePage == PAGE_VISUALS) activeView = visualsMenuView;
        else if (activePage == PAGE_ZONES) activeView = zonesMenuView;
        else if (activePage == PAGE_DATA) activeView = dataMenuView;
        else if (activePage == PAGE_DEV) activeView = devMenuView;
    }
    void executeMenuEdit(int dir) {
        if (activeView) activeView->executeMenuEdit(this, dir);
    }
};

#endif


inline void MainMenuView::handleMenuClick(UIManager* ui) {
    String selItem = String(ui->currentMenuItems[ui->menuSelection]);
    if (selItem.startsWith("VISUAL SETTINGS") || selItem.startsWith("  [DISPLAY/HUD]")) { ui->activePage = PAGE_VISUALS; ui->menuSelection = 0; }
    else if (selItem.startsWith("ZONE CONFIG") || selItem.startsWith("  [BOUNDARIES]")) { ui->activePage = PAGE_ZONES; ui->menuSelection = 0; }
    else if (selItem.startsWith("TARGET DATA") || selItem.startsWith("  [GAIN/FILTER]")) { ui->activePage = PAGE_DATA; ui->menuSelection = 0; }
    else if (selItem.startsWith("DEV OPTIONS")) { ui->activePage = PAGE_DEV; ui->menuSelection = 0; }
    else if (selItem.startsWith("USER GUIDE")) { ui->state = STATE_GUIDE; ui->guidePage = 0; }
    else if (selItem.startsWith("[ Exit Menu ]")) { ui->state = STATE_RADAR_VIEW; }
}
inline void MainMenuView::executeMenuEdit(UIManager* ui, int dir) {}
inline void MainMenuView::populateMenuPage(UIManager* ui, char items[][32], int& numItems) { ui->populateMainMenu(items, numItems); }

inline void VisualsMenuView::handleMenuClick(UIManager* ui) {
    String selItem = String(ui->currentMenuItems[ui->menuSelection]);
    if (selItem.startsWith("<- Back")) { ui->activePage = PAGE_MAIN; ui->menuSelection = 0; }
    else { ui->state = STATE_MENU_EDIT; }
}
inline void VisualsMenuView::executeMenuEdit(UIManager* ui, int dir) {
    String selItem = String(ui->currentMenuItems[ui->menuSelection]);
    if (selItem.startsWith("Theme:")) {
        int t = (int)ui->theme + dir;
        if (t > 4) t = 0; if (t < 0) t = 4;
        ui->theme = (ThemeStyle)t;
        applyTheme(t);
        ui->updateThemeText();
        if (ui->theme == THEME_ALIEN) { ui->sweepLineEnabled = true; ui->trailLength = 8; ui->gridEnabled = true; }
        else if (ui->theme == THEME_MINIMAL) { ui->sweepLineEnabled = false; ui->trailLength = 0; ui->gridEnabled = true; }
        else { ui->sweepLineEnabled = true; ui->trailLength = 3; ui->gridEnabled = true; }
        return;
    }
    if (selItem.startsWith("Icon:")) {
        int ic = (int)ui->targetIcon + dir;
        if (ic > 3) ic = 0; if (ic < 0) ic = 3;
        ui->targetIcon = (TargetIcon)ic;
        return;
    }
    if (selItem.startsWith("Text Size:")) { ui->uiTextSize += dir; if (ui->uiTextSize < 1) ui->uiTextSize = 1; if (ui->uiTextSize > 2) ui->uiTextSize = 2; return; }
    if (selItem.startsWith("UI Scale:")) { ui->uiScale += dir * 0.1f; if (ui->uiScale < 0.5f) ui->uiScale = 0.5f; if (ui->uiScale > 2.0f) ui->uiScale = 2.0f; return; }
    if (selItem.startsWith("Sweep Line:")) { ui->sweepLineEnabled = !ui->sweepLineEnabled; return; }
    if (selItem.startsWith("Sweep Mode:")) { ui->simulatedSweep = !ui->simulatedSweep; return; }
    if (selItem.startsWith("Trails:")) { ui->trailLength += dir; if (ui->trailLength < 0) ui->trailLength = 0; if (ui->trailLength > 10) ui->trailLength = 10; return; }
    if (selItem.startsWith("Grid:")) { ui->gridEnabled = !ui->gridEnabled; return; }
    if (selItem.startsWith("Boot Anim:")) { ui->startupAnimEnabled = !ui->startupAnimEnabled; return; }
}
inline void VisualsMenuView::populateMenuPage(UIManager* ui, char items[][32], int& numItems) { ui->populateVisualsMenu(items, numItems); }

inline void ZonesMenuView::handleMenuClick(UIManager* ui) {
    String selItem = String(ui->currentMenuItems[ui->menuSelection]);
    if (selItem.startsWith("<- Back")) { ui->activePage = PAGE_MAIN; ui->menuSelection = 0; }
    else { ui->state = STATE_MENU_EDIT; }
}
inline void ZonesMenuView::executeMenuEdit(UIManager* ui, int dir) {
    String selItem = String(ui->currentMenuItems[ui->menuSelection]);
    if (selItem.startsWith("Warn Zone:")) {
        int p = (int)ui->zoneManager.getWarnPreset() + dir;
        if (p > 4) p = 0; if (p < 0) p = 4;
        ui->zoneManager.setWarnPreset((ZonePreset)p);
        return;
    }
    if (ui->zoneManager.getWarnPreset() == ZONE_CUSTOM) {
        RadialZone z = ui->zoneManager.getWarnCustom();
        if (selItem.startsWith(" W-MinD:")) { z.minDist += dir * 100; if(z.minDist < 0) z.minDist=0; ui->zoneManager.setWarnCustom(z); return; }
        if (selItem.startsWith(" W-MaxD:")) { z.maxDist += dir * 100; if(z.maxDist < z.minDist) z.maxDist=z.minDist; ui->zoneManager.setWarnCustom(z); return; }
        if (selItem.startsWith(" W-MinA:")) { z.minAngle += dir * 5; if(z.minAngle < -90) z.minAngle=-90; ui->zoneManager.setWarnCustom(z); return; }
        if (selItem.startsWith(" W-MaxA:")) { z.maxAngle += dir * 5; if(z.maxAngle > 90) z.maxAngle=90; ui->zoneManager.setWarnCustom(z); return; }
    }
    if (ui->zoneManager.getWarnPreset() != ZONE_OFF) {
        if (selItem.startsWith("Warn Fuzz:")) {
            int f = ui->zoneManager.getFuzzingThreshold() + dir * 5;
            if (f < 0) f = 0; if (f > 100) f = 100;
            ui->zoneManager.setFuzzingThreshold(f);
            return;
        }
        if (selItem.startsWith("Warn Time:")) {
            ui->zoneManager.setHistoryWindow(ui->zoneManager.getHistoryWindow() + dir);
            return;
        }
    }
    if (selItem.startsWith("Dead Zone:")) {
        int p = (int)ui->zoneManager.getDeadPreset() + dir;
        if (p > 4) p = 0; if (p < 0) p = 4;
        ui->zoneManager.setDeadPreset((ZonePreset)p);
        return;
    }
    if (ui->zoneManager.getDeadPreset() == ZONE_CUSTOM) {
        RadialZone z = ui->zoneManager.getDeadCustom();
        if (selItem.startsWith(" D-MinD:")) { z.minDist += dir * 100; if(z.minDist < 0) z.minDist=0; ui->zoneManager.setDeadCustom(z); return; }
        if (selItem.startsWith(" D-MaxD:")) { z.maxDist += dir * 100; if(z.maxDist < z.minDist) z.maxDist=z.minDist; ui->zoneManager.setDeadCustom(z); return; }
        if (selItem.startsWith(" D-MinA:")) { z.minAngle += dir * 5; if(z.minAngle < -90) z.minAngle=-90; ui->zoneManager.setDeadCustom(z); return; }
        if (selItem.startsWith(" D-MaxA:")) { z.maxAngle += dir * 5; if(z.maxAngle > 90) z.maxAngle=90; ui->zoneManager.setDeadCustom(z); return; }
    }
}
inline void ZonesMenuView::populateMenuPage(UIManager* ui, char items[][32], int& numItems) { ui->populateZonesMenu(items, numItems); }

inline void DataMenuView::handleMenuClick(UIManager* ui) {
    String selItem = String(ui->currentMenuItems[ui->menuSelection]);
    if (selItem.startsWith("<- Back")) { ui->activePage = PAGE_MAIN; ui->menuSelection = 0; }
    else if (selItem.startsWith("[ Reset Tracking ]")) { ui->actionRequested = 1; ui->state = STATE_RADAR_VIEW; }
    else { ui->state = STATE_MENU_EDIT; }
}
inline void DataMenuView::executeMenuEdit(UIManager* ui, int dir) {
    String selItem = String(ui->currentMenuItems[ui->menuSelection]);
    if (selItem.startsWith("Telemetry:")) {
        int tm = (int)ui->telemetryMode + dir;
        if (tm > 4) tm = 0; if (tm < 0) tm = 4;
        ui->telemetryMode = (TelemetryMode)tm;
        return;
    }
    if (selItem.startsWith("Sensitivity:")) { ui->sensitivity += dir; if (ui->sensitivity < 1) ui->sensitivity = 10; if (ui->sensitivity > 10) ui->sensitivity = 10; return; }
    if (selItem.startsWith("Loc Avg:")) { ui->locationAveraging += dir; if (ui->locationAveraging < 1) ui->locationAveraging = 1; if (ui->locationAveraging > 10) ui->locationAveraging = 10; return; }
    if (selItem.startsWith("Smoothing:")) {
        ui->interpolationAmount += (dir * 0.1f);
        if (ui->interpolationAmount < 0.1f) ui->interpolationAmount = 0.1f;
        if (ui->interpolationAmount > 1.05f) ui->interpolationAmount = 1.0f;
        return;
    }
}
inline void DataMenuView::populateMenuPage(UIManager* ui, char items[][32], int& numItems) { ui->populateDataMenu(items, numItems); }

inline void DevMenuView::handleMenuClick(UIManager* ui) {
    String selItem = String(ui->currentMenuItems[ui->menuSelection]);
    if (ui->menuSelection == 0) { ui->activePage = PAGE_MAIN; ui->menuSelection = 0; }
    else if (ui->menuSelection == ui->maxMenuSelection - 5 && ui->devRiskAccepted) { ui->actionRequested = 5; ui->state = STATE_SELF_TEST; ui->selfTestDone = false; }
    else if (ui->menuSelection == ui->maxMenuSelection - 4 && ui->devRiskAccepted) { ui->state = STATE_CONFIRM_RESET; }
    else if (ui->menuSelection == ui->maxMenuSelection - 3 && ui->devRiskAccepted) { ui->actionRequested = 2; ui->state = STATE_RADAR_VIEW; }
    else if (ui->menuSelection == ui->maxMenuSelection - 2 && ui->devRiskAccepted) { ui->state = STATE_IMPORTING; }
    else if (ui->menuSelection == ui->maxMenuSelection - 1 && ui->devRiskAccepted) {
        ui->currentWifiPass = BroadcastServer::getWiFiPassword();
        if (ui->currentWifiPass == "") ui->currentWifiPass = "Not Set";
        ui->state = STATE_VIEW_WIFI;
    }
    else if (String(ui->currentMenuItems[ui->menuSelection]).startsWith("[ REGEN WIFI PASS ]") && ui->devRiskAccepted) { ui->state = STATE_CONFIRM_WIFI_GEN; }
    else { ui->state = STATE_MENU_EDIT; }
}
inline void DevMenuView::executeMenuEdit(UIManager* ui, int dir) {
    String selItem = String(ui->currentMenuItems[ui->menuSelection]);
    if (selItem.startsWith("Accept Risk?")) { ui->devRiskAccepted = !ui->devRiskAccepted; return; }
    if (ui->devRiskAccepted) {
        if (selItem.startsWith("Motion Comp:")) { ui->motionCompEnabled = !ui->motionCompEnabled; return; }
        if (selItem.startsWith("Passthrough:")) { ui->passthroughMode = !ui->passthroughMode; return; }
        if (selItem.startsWith("Show StdDev:")) { ui->showStdDev = !ui->showStdDev; return; }
        if (selItem.startsWith("[ FACTORY RESET ]")) {
            ui->state = STATE_CONFIRM_RESET;
            return;
        }
    }
}
inline void DevMenuView::populateMenuPage(UIManager* ui, char items[][32], int& numItems) { ui->populateDevMenu(items, numItems); }

#endif
