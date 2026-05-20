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
    STATE_FALLBACK,
    STATE_CONFIRM_RESET,
    STATE_CONFIRM_WIFI_GEN,
    STATE_VIEW_WIFI_PASS
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
    bool devRiskAccepted = false;
    bool motionCompEnabled = true;
    bool passthroughMode = true;
    bool broadcastModeEnabled = false;
    bool showStdDev = false;
    int uiTextSize = 1;

    UIManager(TFT_eSPI& display) : tft(display), sprite(&display) {
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

    void loadSettings() {
        preferences.begin("radar_ui", false);
        theme = (ThemeStyle)preferences.getInt("theme", THEME_ALIEN);
        targetIcon = (TargetIcon)preferences.getInt("icon", ICON_SMART);
        sweepLineEnabled = preferences.getBool("sweep", true);
        trailLength = preferences.getInt("trails", 5);
        gridEnabled = preferences.getBool("grid", true);
        startupAnimEnabled = preferences.getBool("startup", true);
        simulatedSweep = preferences.getBool("simSwp", false);
        showStdDev = preferences.getBool("showStd", false);

        telemetryMode = (TelemetryMode)preferences.getInt("tData", TELEMETRY_OFF);
        uiTextSize = preferences.getInt("textSize", 1);
        sensitivity = preferences.getInt("sens", 5);
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
        if (state == STATE_CONFIRM_RESET,
    STATE_CONFIRM_WIFI_GEN) {
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
        } else if (state == STATE_CONFIRM_RESET,
    STATE_CONFIRM_WIFI_GEN) {
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

    void handleButton() {

        if (state == STATE_CONFIRM_RESET,
    STATE_CONFIRM_WIFI_GEN) {
            sprite.fillSprite(themeDanger);
            sprite.setTextColor(TFT_WHITE);
            sprite.setCursor(10, 100);
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
                newPass += charset[random(0, sizeof(charset) - 1)];
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
                } else if (state == STATE_CONFIRM_WIFI_GEN) {
            sprite.fillSprite(themeWarning);
            sprite.setTextColor(themeBg);
            sprite.setCursor(10, 100);
            sprite.print("REGEN WIFI PASS...");
            sprite.pushSprite(0, 0);

            const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
            String newPass = "";
            for (int i = 0; i < 12; i++) {
                newPass += charset[random(0, sizeof(charset) - 1)];
            }
            preferences.begin("radar_sys", false);
            preferences.putString("wifi_pass", newPass);
            preferences.end();

            sprite.fillSprite(themeWarning);
            sprite.setTextColor(themeBg);
            sprite.setCursor(10, 100);
            sprite.print("NEW PASS:");
            sprite.setCursor(10, 115);
            sprite.print(newPass);
            sprite.pushSprite(0, 0);

            delay(3000);
            ESP.restart();
            return;
                } else if (state == STATE_CONFIRM_WIFI_GEN) {
            sprite.fillSprite(themeWarning);
            sprite.setTextColor(themeBg);
            sprite.setCursor(10, 100);
            sprite.print("REGEN WIFI PASS...");
            sprite.pushSprite(0, 0);

            const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
            String newPass = "";
            for (int i = 0; i < 12; i++) {
                newPass += charset[random(0, sizeof(charset) - 1)];
            }
            preferences.begin("radar_sys", false);
            preferences.putString("wifi_pass", newPass);
            preferences.end();

            sprite.fillSprite(themeWarning);
            sprite.setTextColor(themeBg);
            sprite.setCursor(10, 100);
            sprite.print("NEW PASS:");
            sprite.setCursor(10, 115);
            sprite.print(newPass);
            sprite.pushSprite(0, 0);

            delay(3000);
            ESP.restart();
            return;
                } else if (state == STATE_VIEW_WIFI_PASS) {
            state = STATE_MENU;
            return;
                } else if (state == STATE_VIEW_WIFI_PASS) {
            sprite.fillRect(10, 90, 220, 60, themePrimary);
            sprite.setTextColor(themeBg);
            sprite.setCursor(20, 100);
            sprite.print("CURRENT WIFI PASS:");

            preferences.begin("radar_sys", true);
            String wifiPass = preferences.getString("wifi_pass", "NOT SET");
            preferences.end();

            sprite.setCursor(20, 115);
            sprite.print(wifiPass);
            sprite.setCursor(20, 130);
            sprite.print("[PRESS] TO CLOSE");
        } else if (state == STATE_IMPORTING) {
            actionRequested = 4; // Apply batched changes
            state = STATE_MENU;
        } else if (state == STATE_FALLBACK) {
            actionRequested = 3; // Confirm fallback
            state = STATE_RADAR_VIEW;
        } else if (state == STATE_CONFIRM_RESET,
    STATE_CONFIRM_WIFI_GEN) {
            sprite.fillSprite(0xFDB5); // themeDanger
            sprite.setTextColor(TFT_WHITE);
            sprite.setCursor(10, 100);
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
            targetVelX[index] = vx * (tft.width() / 2) / 5000;
            targetVelY[index] = -vy * tft.width() / 5000; // Y is inverted on screen
            targetAccX[index] = ax * (tft.width() / 2) / 5000;
            targetAccY[index] = -ay * tft.width() / 5000;
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
                    targetGoalX[i] = (tft.width() / 2) + (targets[i].x * (tft.width() / 2) / 5000);
                    targetGoalY[i] = tft.height() - (targets[i].y * tft.height() / 5000);

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
            sprite.setTextColor(themePrimary, themeBg);
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
            sprite.setTextColor(themePrimary, themeBg);
            sprite.print("Navigate via the dial:");

            sprite.setCursor(20, 80); sprite.print("- TURN: Scroll/Adjust");
            sprite.setCursor(20, 110); sprite.print("- PRESS: Select/Enter");
            sprite.setCursor(20, 140); sprite.print("- HOLD: Info Tooltips");



        } else if (guidePage == 2) {
            sprite.print("GUIDE 3/3: ZONES");
            sprite.setCursor(10, 40);
            sprite.setTextColor(themePrimary, themeBg);
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
        if (state == STATE_GUIDE) {
            drawGuideScreen();
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
            sprite.setCursor(85, 116);
            sprite.print("NO CONTACTS");
        }

        drawHUD();

        if (state == STATE_MENU || state == STATE_MENU_EDIT) {
            drawMenuOverlay();
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
                newPass += charset[random(0, sizeof(charset) - 1)];
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
                } else if (state == STATE_CONFIRM_WIFI_GEN) {
            sprite.fillSprite(themeWarning);
            sprite.setTextColor(themeBg);
            sprite.setCursor(10, 100);
            sprite.print("REGEN WIFI PASS...");
            sprite.pushSprite(0, 0);

            const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
            String newPass = "";
            for (int i = 0; i < 12; i++) {
                newPass += charset[random(0, sizeof(charset) - 1)];
            }
            preferences.begin("radar_sys", false);
            preferences.putString("wifi_pass", newPass);
            preferences.end();

            sprite.fillSprite(themeWarning);
            sprite.setTextColor(themeBg);
            sprite.setCursor(10, 100);
            sprite.print("NEW PASS:");
            sprite.setCursor(10, 115);
            sprite.print(newPass);
            sprite.pushSprite(0, 0);

            delay(3000);
            ESP.restart();
            return;
                } else if (state == STATE_CONFIRM_WIFI_GEN) {
            sprite.fillSprite(themeWarning);
            sprite.setTextColor(themeBg);
            sprite.setCursor(10, 100);
            sprite.print("REGEN WIFI PASS...");
            sprite.pushSprite(0, 0);

            const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
            String newPass = "";
            for (int i = 0; i < 12; i++) {
                newPass += charset[random(0, sizeof(charset) - 1)];
            }
            preferences.begin("radar_sys", false);
            preferences.putString("wifi_pass", newPass);
            preferences.end();

            sprite.fillSprite(themeWarning);
            sprite.setTextColor(themeBg);
            sprite.setCursor(10, 100);
            sprite.print("NEW PASS:");
            sprite.setCursor(10, 115);
            sprite.print(newPass);
            sprite.pushSprite(0, 0);

            delay(3000);
            ESP.restart();
            return;
                } else if (state == STATE_VIEW_WIFI_PASS) {
            state = STATE_MENU;
            return;
                } else if (state == STATE_VIEW_WIFI_PASS) {
            sprite.fillRect(10, 90, 220, 60, themePrimary);
            sprite.setTextColor(themeBg);
            sprite.setCursor(20, 100);
            sprite.print("CURRENT WIFI PASS:");

            preferences.begin("radar_sys", true);
            String wifiPass = preferences.getString("wifi_pass", "NOT SET");
            preferences.end();

            sprite.setCursor(20, 115);
            sprite.print(wifiPass);
            sprite.setCursor(20, 130);
            sprite.print("[PRESS] TO CLOSE");
        } else if (state == STATE_IMPORTING) {
            float pulse = (sinf(millis() * 0.005f) + 1.0f) * 0.5f;
            uint16_t pulseColor = sprite.alphaBlend((uint8_t)(pulse * 100.0f) + 155, themeWarning, themeBg);
            sprite.fillRect(10, 100, 220, 40, pulseColor);
            sprite.setTextColor(themeBg, pulseColor);
            sprite.setCursor(20, 110);

            int dots = (millis() / 500) % 4;
            const char* dotStr = (dots == 0) ? "" : (dots == 1) ? "." : (dots == 2) ? ".." : "...";
            sprite.printf("WAITING FOR CONFIG%-3s", dotStr);

            sprite.setCursor(20, 125);
            sprite.print("[PRESS BUTTON TO APPLY]");
        } else if (state == STATE_FALLBACK) {
            float pulse = (sinf(millis() * 0.005f) + 1.0f) * 0.5f;
            uint16_t pulseColor = sprite.alphaBlend((uint8_t)(pulse * 100.0f) + 155, themeDanger, themeBg);
            sprite.fillRect(10, 90, 220, 60, pulseColor);
            sprite.setTextColor(themePrimary, pulseColor);
            sprite.setCursor(20, 100);
            sprite.print("NEW CONFIG LOADED");
            sprite.setCursor(20, 115);
            sprite.print("PRESS BUTTON TO KEEP");
            sprite.setCursor(20, 130);

            int dots = (millis() / 500) % 4;
            const char* dotStr = (dots == 0) ? "" : (dots == 1) ? "." : (dots == 2) ? ".." : "...";
            sprite.printf("OR WAIT TO REVERT%-3s", dotStr);
        } else if (state == STATE_CONFIRM_RESET,
    STATE_CONFIRM_WIFI_GEN) {
            float pulse = (sinf(millis() * 0.005f) + 1.0f) * 0.5f;
            uint16_t pulseColor = sprite.alphaBlend((uint8_t)(pulse * 100.0f) + 155, themeDanger, themeBg);
            sprite.fillRect(10, 90, 220, 60, pulseColor);
            sprite.setTextColor(TFT_WHITE, pulseColor);
            sprite.setCursor(20, 100);
            sprite.print("CONFIRM FACTORY RESET");
            sprite.setCursor(20, 115);
            sprite.print("[PRESS] TO WIPE");
            sprite.setCursor(20, 130);
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
                int tx = (tft.width() / 2) + 180 * cosf(tr);
                int ty = tft.height() + 180 * sinf(tr);
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
            if (!targetCoasting[i]) sprite.drawCircle(cx, cy, 5, themePrimary);
        }
        else if (targetIcon == ICON_SQUARE) {
            sprite.fillRect(cx - 3, cy - 3, 7, 7, color);
            if (!targetCoasting[i]) sprite.drawRect(cx - 4, cy - 4, 9, 9, themePrimary);
        }
        else if (targetIcon == ICON_TRIANGLE) {
            sprite.fillTriangle(cx, cy - 5, cx - 4, cy + 3, cx + 4, cy + 3, color);
            if (!targetCoasting[i]) sprite.drawTriangle(cx, cy - 6, cx - 5, cy + 4, cx + 5, cy + 4, themePrimary);
        }
        else if (targetIcon == ICON_SMART) {
            sprite.drawCircle(cx, cy, 3, color);
            if (!targetCoasting[i]) sprite.drawCircle(cx, cy, 4, themePrimary);

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

                    // ⚡ Bolt: Replace atan2f and cosf/sinf with fixed vector rotation.
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
                float screenRadius = targetStdDev[i] * (120.0f * 0.0002f);
                if (screenRadius < 2.0f) screenRadius = 2.0f;
                if (screenRadius > 120.0f) screenRadius = tft.width() / 2.0f;
                uint16_t devColor = sprite.alphaBlend(100, baseColor, themeBg); // subtle wireframe
                sprite.drawCircle(cx, cy, (int)screenRadius, devColor);
            }

            if (zoneManager.isWarning(i)) {
                float pulse = (sinf(millis() * 0.0066667f) + 1.0f) * 0.5f;
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

            if (targetIcon == ICON_CIRCLE) {
                sprite.fillCircle(cx, cy, 4, color);
                if (!targetCoasting[i]) sprite.drawCircle(cx, cy, 5, themePrimary);
            }
            else if (targetIcon == ICON_SQUARE) {
                sprite.fillRect(cx - 3, cy - 3, 7, 7, color);
                if (!targetCoasting[i]) sprite.drawRect(cx - 4, cy - 4, 9, 9, themePrimary);
            }
            else if (targetIcon == ICON_TRIANGLE) {
                sprite.fillTriangle(cx, cy - 5, cx - 4, cy + 3, cx + 4, cy + 3, color);
                if (!targetCoasting[i]) sprite.drawTriangle(cx, cy - 6, cx - 5, cy + 4, cx + 5, cy + 4, themePrimary);
            }
            else if (targetIcon == ICON_SMART) {
                sprite.drawCircle(cx, cy, 3, color);
                if (!targetCoasting[i]) sprite.drawCircle(cx, cy, 4, themePrimary);

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
                    float stickLen = 5.0f + (smoothSpeed[i] * 0.1f);
                    if (stickLen > 25.0f) stickLen = 25.0f;
                    float sLen = sqrtf(smoothVecX[i]*smoothVecX[i] + smoothVecY[i]*smoothVecY[i]);
                    if (sLen > 0.01f) {
                        float nSvx = smoothVecX[i] / sLen;
                        float nSvy = smoothVecY[i] / sLen;
                        int ex = cx + (int)(nSvx * stickLen);
                        int ey = cy + (int)(nSvy * stickLen);
                        sprite.drawLine(cx, cy, ex, ey, color);

                        // ⚡ Bolt: Replace atan2f and cosf/sinf with fixed vector rotation.
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
                    int angle = (int)(atan2f((float)rawTargetX[i], (float)rawTargetY[i]) * 180.0f / PI);
                    sprite.printf("%.1fm %ddeg", dist_m, angle);
                } else if (telemetryMode == TELEMETRY_VELOCITY) {
                    float speed_ms = (float)rawTargetSpeed[i] * 0.1f;
                    sprite.printf("%.1fm/s", speed_ms);
                } else if (telemetryMode == TELEMETRY_RAW) {
                    sprite.printf("%dmm,%dmm", rawTargetX[i], rawTargetY[i]);
                } else if (telemetryMode == TELEMETRY_ALL) {
                    float dist_m = sqrtf((long)rawTargetX[i]*rawTargetX[i] + (long)rawTargetY[i]*rawTargetY[i]) * 0.001f;
                    int angle = (int)(atan2f((float)rawTargetX[i], (float)rawTargetY[i]) * 180.0f / PI);
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
public:
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

        // ⚡ Bolt: Hoist trigonometry out of radial rendering loops
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
            sprite.drawLine((tft.width() / 2) + minR*cosA, tft.width() + minR*sinA, (tft.width() / 2) + maxR*cosA, tft.width() + maxR*sinA, color);
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
            sprite.drawLine(0, tft.width() / 2, tft.width(), tft.width() / 2, gridColor);
            sprite.drawLine(tft.width() / 2, 16, tft.width() / 2, tft.height() - 16, gridColor); // Avoid drawing over top/bottom bars

            // Faint concentric circles
            for (int r = 40; r <= 100; r += 30) {
                sprite.drawRect((tft.width() / 2) - r, (tft.width() / 2) - r, r * 2, r * 2, sprite.alphaBlend(50, themePrimary, themeBg));
            }
            if (theme == THEME_ALIEN) {
                // ⚡ Bolt: Hoist trigonometry out of radial rendering loops
                for (int a=0; a<=180; a+=5) {
                    float rad = (a - 180) * 0.0174533f;
                    float cosA = cosf(rad);
                    float sinA = sinf(rad);
                    for (int r=60; r<=180; r+=60) {
                        sprite.drawPixel((tft.width() / 2) + r * cosA, tft.width() + r * sinA, gridColor);
                    }
                }
            } else {
                sprite.drawRect(60, 180, tft.width() / 2, tft.width() / 2, gridColor);
                sprite.drawRect(0, tft.width() / 2, tft.width(), tft.width(), gridColor);
            }

            // Ticks along axes
            for (int r = 30; r <= 90; r += 30) {
                sprite.drawLine((tft.width() / 2) - 3, (tft.width() / 2) - r, (tft.width() / 2) + 3, (tft.width() / 2) - r, gridColor); // Vertical axis ticks
                sprite.drawLine((tft.width() / 2) - r, (tft.width() / 2) - 3, (tft.width() / 2) - r, (tft.width() / 2) + 3, gridColor); // Horizontal axis ticks
                sprite.drawLine((tft.width() / 2) + r, (tft.width() / 2) - 3, (tft.width() / 2) + r, (tft.width() / 2) + 3, gridColor);
            }
        }
    }


    void populateMainMenu(char items[][32], int& numItems) {
        sprite.setTextColor(themePrimary, themeBg);
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
        sprite.setTextColor(themePrimary, themeBg);
        sprite.setCursor(15, 5); sprite.print("--- VISUAL SETTINGS ---");

        const char* themeStr = (theme == THEME_STANDARD) ? "Standard" : (theme == THEME_ALIEN ? "Alien" : "Minimal");
        const char* iconStr = (targetIcon == ICON_CIRCLE) ? "CIRCLE" :
                         (targetIcon == ICON_SQUARE) ? "SQUARE" :
                         (targetIcon == ICON_TRIANGLE) ? "TRIANGLE" : "SMART";
        snprintf(items[numItems++], 32, "%s", "<- Back");
        snprintf(items[numItems++], 32, "Theme: %s", themeStr);
        snprintf(items[numItems++], 32, "Icon: %s", iconStr);
        snprintf(items[numItems++], 32, "Text Size: %d", uiTextSize);
        snprintf(items[numItems++], 32, "Sweep Line: %s", sweepLineEnabled ? "ON" : "OFF");
        snprintf(items[numItems++], 32, "Sweep Mode: %s", simulatedSweep ? "SIMULATED" : "VISUAL");
        snprintf(items[numItems++], 32, "Trails: %d", trailLength);
        snprintf(items[numItems++], 32, "Grid: %s", gridEnabled ? "ON" : "OFF");
        snprintf(items[numItems++], 32, "Boot Anim: %s", startupAnimEnabled ? "ON" : "OFF");
    }

    void populateZonesMenu(char items[][32], int& numItems) {
        sprite.setTextColor(themePrimary, themeBg);
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
        sprite.setTextColor(themePrimary, themeBg);
        sprite.setCursor(15, 5); sprite.print("--- TARGET DATA ---");

        const char* tDataStr = (telemetryMode == TELEMETRY_OFF) ? "OFF" :
                          (telemetryMode == TELEMETRY_DIST_ANG) ? "DIST/ANG" :
                          (telemetryMode == TELEMETRY_VELOCITY) ? "SPEED" :
                          (telemetryMode == TELEMETRY_RAW) ? "RAW X/Y" : "ALL";
        int interDisp = (int)(interpolationAmount * 10.0f + 0.5f);

        snprintf(items[numItems++], 32, "%s", "<- Back");
        snprintf(items[numItems++], 32, "Telemetry: %s", tDataStr);
        snprintf(items[numItems++], 32, "Sensitivity: %d cm/s", sensitivity);
        snprintf(items[numItems++], 32, "Loc Avg: %d", locationAveraging);
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
            snprintf(items[numItems++], 32, "%s", "[ FACTORY RESET ]");
            snprintf(items[numItems++], 32, "%s", "[ EXPORT CONFIG ]");
            snprintf(items[numItems++], 32, "%s", "[ IMPORT CONFIG ]");
            snprintf(items[numItems++], 32, "%s", "[ REGEN WIFI PASS ]");
            snprintf(items[numItems++], 32, "%s", "[ REGEN WIFI PASS ]");
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

    void drawMenuTooltip(const char* selectedItemText) {
        sprite.fillRect(10, 140, 220, 60, themeBg);
        sprite.drawRect(10, 140, 220, 60, themeWarning);
        sprite.setTextColor(TFT_WHITE, themeBg);
        sprite.setTextSize(uiTextSize);
        sprite.setCursor(15, 145);
        sprite.print("INFO: ");
        sprite.setCursor(15, 160);

        String selItem = String(selectedItemText);

        if (selItem.startsWith("<- Back")) {
            sprite.print("Return to previous menu.");
        } else if (selItem.startsWith("VISUAL SETTINGS")) {
            sprite.print("Colors, icons, & layout.");
        } else if (selItem.startsWith("ZONE CONFIG")) {
            sprite.print("Warning & dead zones.");
        } else if (selItem.startsWith("TARGET DATA")) {
            sprite.print("Data processing & limits.");
        } else if (selItem.startsWith("DEV OPTIONS")) {
            sprite.print("Advanced & experimental.");
        } else if (selItem.startsWith("USER GUIDE")) {
            sprite.print("Help & instructions.");
        } else if (selItem.startsWith("[ Exit Menu ]")) {
            sprite.print("Return to radar view.");
        } else if (selItem.startsWith("Theme:")) {
            sprite.print("Change color palette.");
        } else if (selItem.startsWith("Icon:")) {
            sprite.print("Change target marker.");
        } else if (selItem.startsWith("Text Size:")) {
            sprite.print("UI text scale (1-2).");
        } else if (selItem.startsWith("Sweep Line:")) {
            sprite.print("Toggle scanning line.");
        } else if (selItem.startsWith("Sweep Mode:")) {
            sprite.print("Simulated vs physical.");
        } else if (selItem.startsWith("Trails:")) {
            sprite.print("Target history length.");
        } else if (selItem.startsWith("Grid:")) {
            sprite.print("Toggle background grid.");
        } else if (selItem.startsWith("Boot Anim:")) {
            sprite.print("Toggle startup sequence.");
        } else if (selItem.startsWith("Warn Zone:")) {
            sprite.print("Visual alert area.");
        } else if (selItem.startsWith(" W-MinD:") || selItem.startsWith(" D-MinD:")) {
            sprite.print("Minimum distance (mm).");
        } else if (selItem.startsWith(" W-MaxD:") || selItem.startsWith(" D-MaxD:")) {
            sprite.print("Maximum distance (mm).");
        } else if (selItem.startsWith(" W-MinA:") || selItem.startsWith(" D-MinA:")) {
            sprite.print("Left-most angle (deg).");
        } else if (selItem.startsWith(" W-MaxA:") || selItem.startsWith(" D-MaxA:")) {
            sprite.print("Right-most angle (deg).");
        } else if (selItem.startsWith("Warn Fuzz:")) {
            sprite.print("Boundary tolerance (%).");
        } else if (selItem.startsWith("Warn Time:")) {
            sprite.print("Time to trigger alert.");
        } else if (selItem.startsWith("Dead Zone:")) {
            sprite.print("Ignore targets area.");
        } else if (selItem.startsWith("Telemetry:")) {
            sprite.print("On-screen target data.");
        } else if (selItem.startsWith("Sensitivity:")) {
            sprite.print("Min target speed (cm/s).");
        } else if (selItem.startsWith("Loc Avg:")) {
            sprite.print("Position smoothing frames.");
        } else if (selItem.startsWith("Smoothing:")) {
            sprite.print("Movement interpolation.");
        } else if (selItem.startsWith("[ Reset Tracking ]")) {
            sprite.print("Clear all targets.");
        } else if (selItem.startsWith("Accept Risk?")) {
            sprite.print("Enable advanced features?");
        } else if (selItem.startsWith("Motion Comp:")) {
            sprite.print("Compensate for host movement.");
        } else if (selItem.startsWith("Passthrough:")) {
            sprite.print("Raw UART to serial.");
        } else if (selItem.startsWith("Show StdDev:")) {
            sprite.print("Display data variance.");
        } else if (selItem.startsWith("[ FACTORY RESET ]")) {
            sprite.print("Erase all settings.");
        } else if (selItem.startsWith("[ EXPORT CONFIG ]")) {
            sprite.print("Save settings to SD.");
        } else if (selItem.startsWith("[ IMPORT CONFIG ]")) {
            sprite.print("Load settings from SD.");
        } else {
            sprite.print("Adjust setting value.");
        }
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
                sprite.setTextColor(themePrimary, themeBg);
            }

            sprite.setCursor(15, yPos);
            sprite.print(items[idx]);
        }

        if (numItems > 4) {
            drawMenuScrollbar(numItems, startIdx);
        }

        if (showTooltip) {
            drawMenuTooltip(items[menuSelection]);
            sprite.fillRect(10, 140, 220, 60, themeBg);
            sprite.drawRect(10, 140, 220, 60, themeWarning);
            sprite.setTextColor(themePrimary, themeBg);
            sprite.setTextSize(uiTextSize);
            sprite.setCursor(15, 145);
            sprite.print("INFO: ");
            sprite.setCursor(15, 160);

            String selItem = String(items[menuSelection]);

            if (selItem.startsWith("<- Back")) {
                sprite.print("Return to previous menu.");
            } else if (selItem.startsWith("VISUAL SETTINGS")) {
                sprite.print("Colors, icons, & layout.");
            } else if (selItem.startsWith("  [DISPLAY/HUD]")) {
                sprite.print("Colors, icons, & layout.");
            } else if (selItem.startsWith("ZONE CONFIG")) {
                sprite.print("Warning & dead zones.");
            } else if (selItem.startsWith("  [BOUNDARIES]")) {
                sprite.print("Warning & dead zones.");
            } else if (selItem.startsWith("TARGET DATA")) {
                sprite.print("Data processing & limits.");
            } else if (selItem.startsWith("  [GAIN/FILTER]")) {
                sprite.print("Data processing & limits.");
            } else if (selItem.startsWith("DEV OPTIONS")) {
                sprite.print("Advanced & experimental.");
            } else if (selItem.startsWith("USER GUIDE")) {
                sprite.print("Help & instructions.");
            } else if (selItem.startsWith("[ Exit Menu ]")) {
                sprite.print("Return to radar view.");
            } else if (selItem.startsWith("Theme:")) {
                sprite.print("Change color palette.");
            } else if (selItem.startsWith("Icon:")) {
                sprite.print("Change target marker.");
            } else if (selItem.startsWith("Text Size:")) {
                sprite.print("UI text scale (1-2).");
            } else if (selItem.startsWith("Sweep Line:")) {
                sprite.print("Toggle scanning line.");
            } else if (selItem.startsWith("Sweep Mode:")) {
                sprite.print("Simulated vs physical.");
            } else if (selItem.startsWith("Trails:")) {
                sprite.print("Target history length.");
            } else if (selItem.startsWith("Grid:")) {
                sprite.print("Toggle background grid.");
            } else if (selItem.startsWith("Boot Anim:")) {
                sprite.print("Toggle startup sequence.");
            } else if (selItem.startsWith("Warn Zone:")) {
                sprite.print("Visual alert area.");
            } else if (selItem.startsWith(" W-MinD:") || selItem.startsWith(" D-MinD:")) {
                sprite.print("Minimum distance (mm).");
            } else if (selItem.startsWith(" W-MaxD:") || selItem.startsWith(" D-MaxD:")) {
                sprite.print("Maximum distance (mm).");
            } else if (selItem.startsWith(" W-MinA:") || selItem.startsWith(" D-MinA:")) {
                sprite.print("Left-most angle (deg).");
            } else if (selItem.startsWith(" W-MaxA:") || selItem.startsWith(" D-MaxA:")) {
                sprite.print("Right-most angle (deg).");
            } else if (selItem.startsWith("Warn Fuzz:")) {
                sprite.print("Boundary tolerance (%).");
            } else if (selItem.startsWith("Warn Time:")) {
                sprite.print("Time to trigger alert.");
            } else if (selItem.startsWith("Dead Zone:")) {
                sprite.print("Ignore targets area.");
            } else if (selItem.startsWith("Telemetry:")) {
                sprite.print("On-screen target data.");
            } else if (selItem.startsWith("Sensitivity:")) {
                sprite.print("Min target speed (cm/s).");
            } else if (selItem.startsWith("Loc Avg:")) {
                sprite.print("Position smoothing frames.");
            } else if (selItem.startsWith("Smoothing:")) {
                sprite.print("Movement interpolation.");
            } else if (selItem.startsWith("[ Reset Tracking ]")) {
                sprite.print("Clear all targets.");
            } else if (selItem.startsWith("Accept Risk?")) {
                sprite.print("Enable advanced features?");
            } else if (selItem.startsWith("Motion Comp:")) {
                sprite.print("Compensate for host movement.");
            } else if (selItem.startsWith("Passthrough:")) {
                sprite.print("Raw UART to serial.");
            } else if (selItem.startsWith("Broadcast AP:")) {
                sprite.print("Host a local Wi-Fi network.");
            } else if (selItem.startsWith("Show StdDev:")) {
                sprite.print("Display data variance.");
            } else if (selItem.startsWith("[ FACTORY RESET ]")) {
                sprite.print("Erase all settings.");
            } else if (selItem.startsWith("[ EXPORT CONFIG ]")) {
                sprite.print("Save settings to SD.");
            } else if (selItem.startsWith("[ IMPORT CONFIG ]")) {
                sprite.print("Load settings from SD.");
            } else {
                sprite.print("Adjust setting value.");
            }
        }
    }

    void drawMenuOverlay() {
        if (menuOverlayY < 200) menuOverlayY += 15;

        sprite.fillRect(0, 0, 240, menuOverlayY, sprite.alphaBlend(220, themeBg, themePrimary));
        sprite.drawLine(0, menuOverlayY, 240, menuOverlayY, themePrimary);
        if (menuOverlayY < 200) return;

        sprite.setTextSize(uiTextSize);
        static char items[24][32];
        int numItems = 0;

        populateMenuPage(items, numItems);
        drawMenuItems(items, numItems);
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
    if (ui->menuSelection == 0 || ui->menuSelection == 1) { ui->activePage = PAGE_VISUALS; ui->menuSelection = 0; }
    else if (ui->menuSelection == 2 || ui->menuSelection == 3) { ui->activePage = PAGE_ZONES; ui->menuSelection = 0; }
    else if (ui->menuSelection == 4 || ui->menuSelection == 5) { ui->activePage = PAGE_DATA; ui->menuSelection = 0; }
    else if (ui->menuSelection == 6) { ui->activePage = PAGE_DEV; ui->menuSelection = 0; }
    else if (ui->menuSelection == 7) { ui->state = STATE_GUIDE; ui->guidePage = 0; }
    else if (ui->menuSelection == 8) { ui->state = STATE_RADAR_VIEW; }
}
inline void MainMenuView::executeMenuEdit(UIManager* ui, int dir) {}
inline void MainMenuView::populateMenuPage(UIManager* ui, char items[][32], int& numItems) { ui->populateMainMenu(items, numItems); }

inline void VisualsMenuView::handleMenuClick(UIManager* ui) {
    if (ui->menuSelection == 0) { ui->activePage = PAGE_MAIN; ui->menuSelection = 0; }
    else { ui->state = STATE_MENU_EDIT; }
}
inline void VisualsMenuView::executeMenuEdit(UIManager* ui, int dir) {
    int idx = 1;
    if (idx++ == ui->menuSelection) {
        int t = (int)ui->theme + dir;
        if (t > 2) t = 0; if (t < 0) t = 2;
        ui->theme = (ThemeStyle)t;
        if (ui->theme == THEME_ALIEN) { ui->sweepLineEnabled = true; ui->trailLength = 8; ui->gridEnabled = true; }
        else if (ui->theme == THEME_MINIMAL) { ui->sweepLineEnabled = false; ui->trailLength = 0; ui->gridEnabled = true; }
        else { ui->sweepLineEnabled = true; ui->trailLength = 3; ui->gridEnabled = true; }
        return;
    }
    if (idx++ == ui->menuSelection) {
        int ic = (int)ui->targetIcon + dir;
        if (ic > 3) ic = 0; if (ic < 0) ic = 3;
        ui->targetIcon = (TargetIcon)ic;
        return;
    }
    if (idx++ == ui->menuSelection) { ui->uiTextSize += dir; if (ui->uiTextSize < 1) ui->uiTextSize = 1; if (ui->uiTextSize > 2) ui->uiTextSize = 2; return; }
    if (idx++ == ui->menuSelection) { ui->sweepLineEnabled = !ui->sweepLineEnabled; return; }
    if (idx++ == ui->menuSelection) { ui->simulatedSweep = !ui->simulatedSweep; return; }
    if (idx++ == ui->menuSelection) { ui->trailLength += dir; if (ui->trailLength < 0) ui->trailLength = 0; if (ui->trailLength > 10) ui->trailLength = 10; return; }
    if (idx++ == ui->menuSelection) { ui->gridEnabled = !ui->gridEnabled; return; }
    if (idx++ == ui->menuSelection) { ui->startupAnimEnabled = !ui->startupAnimEnabled; return; }
}
inline void VisualsMenuView::populateMenuPage(UIManager* ui, char items[][32], int& numItems) { ui->populateVisualsMenu(items, numItems); }

inline void ZonesMenuView::handleMenuClick(UIManager* ui) {
    if (ui->menuSelection == 0) { ui->activePage = PAGE_MAIN; ui->menuSelection = 0; }
    else { ui->state = STATE_MENU_EDIT; }
}
inline void ZonesMenuView::executeMenuEdit(UIManager* ui, int dir) {
    int idx = 1;
    if (idx++ == ui->menuSelection) {
        int p = (int)ui->zoneManager.getWarnPreset() + dir;
        if (p > 4) p = 0; if (p < 0) p = 4;
        ui->zoneManager.setWarnPreset((ZonePreset)p);
        return;
    }
    if (ui->zoneManager.getWarnPreset() == ZONE_CUSTOM) {
        RadialZone z = ui->zoneManager.getWarnCustom();
        if (idx++ == ui->menuSelection) { z.minDist += dir * 100; if(z.minDist < 0) z.minDist=0; ui->zoneManager.setWarnCustom(z); return; }
        if (idx++ == ui->menuSelection) { z.maxDist += dir * 100; if(z.maxDist < z.minDist) z.maxDist=z.minDist; ui->zoneManager.setWarnCustom(z); return; }
        if (idx++ == ui->menuSelection) { z.minAngle += dir * 5; if(z.minAngle < -90) z.minAngle=-90; ui->zoneManager.setWarnCustom(z); return; }
        if (idx++ == ui->menuSelection) { z.maxAngle += dir * 5; if(z.maxAngle > 90) z.maxAngle=90; ui->zoneManager.setWarnCustom(z); return; }
    }
    if (ui->zoneManager.getWarnPreset() != ZONE_OFF) {
        if (idx++ == ui->menuSelection) {
            int f = ui->zoneManager.getFuzzingThreshold() + dir * 5;
            if (f < 0) f = 0; if (f > 100) f = 100;
            ui->zoneManager.setFuzzingThreshold(f);
            return;
        }
        if (idx++ == ui->menuSelection) {
            ui->zoneManager.setHistoryWindow(ui->zoneManager.getHistoryWindow() + dir);
            return;
        }
    }
    if (idx++ == ui->menuSelection) {
        int p = (int)ui->zoneManager.getDeadPreset() + dir;
        if (p > 4) p = 0; if (p < 0) p = 4;
        ui->zoneManager.setDeadPreset((ZonePreset)p);
        return;
    }
    if (ui->zoneManager.getDeadPreset() == ZONE_CUSTOM) {
        RadialZone z = ui->zoneManager.getDeadCustom();
        if (idx++ == ui->menuSelection) { z.minDist += dir * 100; if(z.minDist < 0) z.minDist=0; ui->zoneManager.setDeadCustom(z); return; }
        if (idx++ == ui->menuSelection) { z.maxDist += dir * 100; if(z.maxDist < z.minDist) z.maxDist=z.minDist; ui->zoneManager.setDeadCustom(z); return; }
        if (idx++ == ui->menuSelection) { z.minAngle += dir * 5; if(z.minAngle < -90) z.minAngle=-90; ui->zoneManager.setDeadCustom(z); return; }
        if (idx++ == ui->menuSelection) { z.maxAngle += dir * 5; if(z.maxAngle > 90) z.maxAngle=90; ui->zoneManager.setDeadCustom(z); return; }
    }
}
inline void ZonesMenuView::populateMenuPage(UIManager* ui, char items[][32], int& numItems) { ui->populateZonesMenu(items, numItems); }

inline void DataMenuView::handleMenuClick(UIManager* ui) {
    if (ui->menuSelection == 0) { ui->activePage = PAGE_MAIN; ui->menuSelection = 0; }
    else if (ui->menuSelection == ui->maxMenuSelection) { ui->actionRequested = 1; ui->state = STATE_RADAR_VIEW; }
    else { ui->state = STATE_MENU_EDIT; }
}
inline void DataMenuView::executeMenuEdit(UIManager* ui, int dir) {
    int idx = 1;
    if (idx++ == ui->menuSelection) {
        int tm = (int)ui->telemetryMode + dir;
        if (tm > 4) tm = 0; if (tm < 0) tm = 4;
        ui->telemetryMode = (TelemetryMode)tm;
        return;
    }
    if (idx++ == ui->menuSelection) { ui->sensitivity += dir; if (ui->sensitivity < 1) ui->sensitivity = 10; if (ui->sensitivity > 10) ui->sensitivity = 10; return; }
    if (idx++ == ui->menuSelection) { ui->locationAveraging += dir; if (ui->locationAveraging < 1) ui->locationAveraging = 1; if (ui->locationAveraging > 10) ui->locationAveraging = 10; return; }
    if (idx++ == ui->menuSelection) {
        ui->interpolationAmount += (dir * 0.1f);
        if (ui->interpolationAmount < 0.1f) ui->interpolationAmount = 0.1f;
        if (ui->interpolationAmount > 1.05f) ui->interpolationAmount = 1.0f;
        return;
    }
}
inline void DataMenuView::populateMenuPage(UIManager* ui, char items[][32], int& numItems) { ui->populateDataMenu(items, numItems); }

inline void DevMenuView::handleMenuClick(UIManager* ui) {
    if (ui->menuSelection == 0) { ui->activePage = PAGE_MAIN; ui->menuSelection = 0; }
    else if (ui->menuSelection == ui->maxMenuSelection - 3 && ui->devRiskAccepted) { ui->actionRequested = 2; ui->state = STATE_RADAR_VIEW; }
    else if (ui->menuSelection == ui->maxMenuSelection - 2 && ui->devRiskAccepted) { ui->state = STATE_IMPORTING; }
    else if (ui->menuSelection == ui->maxMenuSelection - 1 && ui->devRiskAccepted) { ui->state = STATE_VIEW_WIFI_PASS; }
    else if (ui->menuSelection == ui->maxMenuSelection && ui->devRiskAccepted) { ui->state = STATE_CONFIRM_WIFI_GEN; }
    else if (ui->menuSelection == 4 && ui->devRiskAccepted) { ui->state = STATE_CONFIRM_RESET,
    STATE_CONFIRM_WIFI_GEN; }
    else { ui->state = STATE_MENU_EDIT; }
}
inline void DevMenuView::executeMenuEdit(UIManager* ui, int dir) {
    int idx = 1;
    if (idx++ == ui->menuSelection) { ui->devRiskAccepted = !ui->devRiskAccepted; return; }
    if (ui->devRiskAccepted) {
        if (idx++ == ui->menuSelection) { ui->motionCompEnabled = !ui->motionCompEnabled; return; }
        if (idx++ == ui->menuSelection) { ui->passthroughMode = !ui->passthroughMode; return; }
        if (idx++ == ui->menuSelection) { ui->showStdDev = !ui->showStdDev; return; }
        if (idx++ == ui->menuSelection) {
            ui->state = STATE_CONFIRM_RESET,
    STATE_CONFIRM_WIFI_GEN;
            ui->sprite.fillSprite(0xFDB5); // themeDanger
            ui->sprite.setTextColor(themePrimary);
            ui->sprite.setCursor(10, 100);
            ui->sprite.print("WIPING PREFERENCES...");
            ui->sprite.pushSprite(0, 0);
            ui->preferences.clear();
            delay(1000);
            ESP.restart();
            return;
        }
    }
}
inline void DevMenuView::populateMenuPage(UIManager* ui, char items[][32], int& numItems) { ui->populateDevMenu(items, numItems); }

#endif
