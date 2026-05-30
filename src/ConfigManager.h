#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "UIManager.h"
#include "ZoneManager.h"

class ConfigManager {
public:

    String generateWiFiPassword() {
        const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        String newPass = "";
        for (int i = 0; i < 12; i++) {
            newPass += charset[random(0, sizeof(charset) - 1)];
        }
        prefs.begin("radar_sys", false);
        prefs.putString("wifi_pass", newPass);
        prefs.end();
        return newPass;
    }

private:
    Preferences prefs;

public:
    void loadThemes() {
        numUserThemes = 0;
        userThemes[numUserThemes++] = FALLBACK_THEME;

        prefs.begin("radar_ui", true);
        String themeJson = prefs.getString("theme_data", "");
        prefs.end();

        if (themeJson.length() > 0) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, themeJson);
            if (!error && doc.is<JsonArray>()) {
                JsonArray arr = doc.as<JsonArray>();
                for (JsonVariant v : arr) {
                    if (numUserThemes < 10) {
                        Theme t;
                        t.name = v["name"].as<String>();
                        t.bg = strtol(v["bg"].as<const char*>(), NULL, 16);
                        t.primary = strtol(v["primary"].as<const char*>(), NULL, 16);
                        t.danger = strtol(v["danger"].as<const char*>(), NULL, 16);
                        t.success = strtol(v["success"].as<const char*>(), NULL, 16);
                        t.warning = strtol(v["warning"].as<const char*>(), NULL, 16);
                        t.text = strtol(v["text"].as<const char*>(), NULL, 16);
                        userThemes[numUserThemes++] = t;
                    }
                }
            }
        }

        // If no user themes loaded, load defaults
        if (numUserThemes == 1) {
            Theme defaults[] = {
                {"Standard", 0x1082, 0x06DD, 0xFDB5, 0x2F20, 0xFDCA, TFT_WHITE},
                {"Minimal", 0x1082, 0x06DD, 0xFDB5, 0x2F20, 0xFDCA, 0xC618},
                {"Cyberpunk", 0x18C3, 0x07E0, 0xF800, 0xFFE0, 0xFD20, 0xFFFF},
                {"Synthwave", 0x2008, 0xF81F, 0xFC00, 0x07E0, 0xFFE0, 0xFFFF},
                {"Forest", 0x0164, 0x07E0, 0xF800, 0x2720, 0xFFE0, 0xFFFF},
                {"High Contrast", 0x0000, 0xFFFF, 0xF800, 0x07E0, 0xFFE0, 0xFFFF},
                {"Sunset", 0x1000, 0xFD20, 0xF800, 0x07E0, 0xFC00, 0xFFFF}
            };
            for (Theme t : defaults) {
                userThemes[numUserThemes++] = t;
            }
        }
    }

    void saveThemes() {
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();
        // Skip fallback theme (index 0)
        for (int i = 1; i < numUserThemes; i++) {
            JsonObject obj = arr.add<JsonObject>();
            obj["name"] = userThemes[i].name;

            char hex[10];
            sprintf(hex, "0x%04X", userThemes[i].bg); obj["bg"] = hex;
            sprintf(hex, "0x%04X", userThemes[i].primary); obj["primary"] = hex;
            sprintf(hex, "0x%04X", userThemes[i].danger); obj["danger"] = hex;
            sprintf(hex, "0x%04X", userThemes[i].success); obj["success"] = hex;
            sprintf(hex, "0x%04X", userThemes[i].warning); obj["warning"] = hex;
            sprintf(hex, "0x%04X", userThemes[i].text); obj["text"] = hex;
        }
        String out;
        serializeJson(doc, out);

        prefs.begin("radar_ui", false);
        prefs.putString("theme_data", out);
        prefs.end();
    }
    void exportConfig(UIManager& ui) {
        JsonDocument doc;

        // Export UI Settings
        prefs.begin("radar_ui", true);
        doc["theme"] = prefs.getInt("theme", 1);
        doc["icon"] = prefs.getInt("icon", ICON_SMART);
        doc["sweep"] = prefs.getBool("sweep", true);
        doc["trails"] = prefs.getInt("trails", 5);
        doc["grid"] = prefs.getBool("grid", true);
        doc["startup"] = prefs.getBool("startup", true);
        doc["simSwp"] = prefs.getBool("simSwp", false);
        doc["tData"] = prefs.getInt("tData", TELEMETRY_OFF);
        doc["textSize"] = prefs.getInt("textSize", 1);
        doc["sens"] = prefs.getInt("sens", 5);
        doc["locAvg"] = prefs.getInt("locAvg", 5);
        doc["interp"] = prefs.getInt("interp", 5);
        prefs.end();

        // Export Zone Settings
        prefs.begin("radar_zones", true);
        doc["warnP"] = prefs.getInt("warnP", ZONE_OFF);
        doc["deadP"] = prefs.getInt("deadP", ZONE_OFF);
        doc["wC_minD"] = prefs.getInt("wC_minD", 1000);
        doc["wC_maxD"] = prefs.getInt("wC_maxD", 3000);
        doc["wC_minA"] = prefs.getInt("wC_minA", -30);
        doc["wC_maxA"] = prefs.getInt("wC_maxA", 30);
        doc["dC_minD"] = prefs.getInt("dC_minD", 0);
        doc["dC_maxD"] = prefs.getInt("dC_maxD", 1000);
        doc["dC_minA"] = prefs.getInt("dC_minA", -90);
        doc["dC_maxA"] = prefs.getInt("dC_maxA", 90);
        doc["fuzzT"] = prefs.getInt("fuzzT", 50);
        doc["histW"] = prefs.getInt("histW", 10);
        prefs.end();

        Serial.println("\n---BEGIN CONFIG---");
        serializeJson(doc, Serial);
        Serial.println("\n---END CONFIG---");
    }

    bool importConfig(const String& jsonStr) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, jsonStr);
        if (error) {
            Serial.print("JSON Parse Error: ");
            Serial.println(error.c_str());
            return false;
        }

        // Backup current to fallback only if not already pending
        if (!isFallbackPending()) {
            backupToFallback();
        }

        // Write new config to main
        prefs.begin("radar_ui", false);
        if(doc["theme"].is<int>()) prefs.putInt("theme", doc["theme"].as<int>());
        if(doc["icon"].is<int>()) prefs.putInt("icon", doc["icon"].as<int>());
        if(doc["sweep"].is<bool>()) prefs.putBool("sweep", doc["sweep"].as<bool>());
        if(doc["trails"].is<int>()) prefs.putInt("trails", doc["trails"].as<int>());
        if(doc["grid"].is<bool>()) prefs.putBool("grid", doc["grid"].as<bool>());
        if(doc["startup"].is<bool>()) prefs.putBool("startup", doc["startup"].as<bool>());
        if(doc["simSwp"].is<bool>()) prefs.putBool("simSwp", doc["simSwp"].as<bool>());
        if(doc["tData"].is<int>()) prefs.putInt("tData", doc["tData"].as<int>());
        if(doc["textSize"].is<int>()) prefs.putInt("textSize", doc["textSize"].as<int>());
        if(doc["sens"].is<int>()) prefs.putInt("sens", doc["sens"].as<int>());
        if(doc["locAvg"].is<int>()) prefs.putInt("locAvg", doc["locAvg"].as<int>());
        if(doc["interp"].is<int>()) prefs.putInt("interp", doc["interp"].as<int>());
        prefs.end();

        prefs.begin("radar_zones", false);
        if(doc["warnP"].is<int>()) prefs.putInt("warnP", doc["warnP"].as<int>());
        if(doc["deadP"].is<int>()) prefs.putInt("deadP", doc["deadP"].as<int>());
        if(doc["wC_minD"].is<int>()) prefs.putInt("wC_minD", doc["wC_minD"].as<int>());
        if(doc["wC_maxD"].is<int>()) prefs.putInt("wC_maxD", doc["wC_maxD"].as<int>());
        if(doc["wC_minA"].is<int>()) prefs.putInt("wC_minA", doc["wC_minA"].as<int>());
        if(doc["wC_maxA"].is<int>()) prefs.putInt("wC_maxA", doc["wC_maxA"].as<int>());
        if(doc["dC_minD"].is<int>()) prefs.putInt("dC_minD", doc["dC_minD"].as<int>());
        if(doc["dC_maxD"].is<int>()) prefs.putInt("dC_maxD", doc["dC_maxD"].as<int>());
        if(doc["dC_minA"].is<int>()) prefs.putInt("dC_minA", doc["dC_minA"].as<int>());
        if(doc["dC_maxA"].is<int>()) prefs.putInt("dC_maxA", doc["dC_maxA"].as<int>());
        if(doc["fuzzT"].is<int>()) prefs.putInt("fuzzT", doc["fuzzT"].as<int>());
        if(doc["histW"].is<int>()) prefs.putInt("histW", doc["histW"].as<int>());
        prefs.end();

        // Flag fallback pending
        prefs.begin("radar_sys", false);
        prefs.putBool("fb_pend", true);
        prefs.end();

        Serial.println("Import applied to memory. Waiting for user action...");
        return true;
    }

    void backupToFallback() {
        prefs.begin("radar_ui", true);
        int theme = prefs.getInt("theme", 1);
        int icon = prefs.getInt("icon", ICON_SMART);
        bool sweep = prefs.getBool("sweep", true);
        int trails = prefs.getInt("trails", 5);
        bool grid = prefs.getBool("grid", true);
        bool startup = prefs.getBool("startup", true);
        bool simSwp = prefs.getBool("simSwp", false);
        int tData = prefs.getInt("tData", TELEMETRY_OFF);
        int textSize = prefs.getInt("textSize", 1);
        int sens = prefs.getInt("sens", 5);
        int locAvg = prefs.getInt("locAvg", 5);
        int interp = prefs.getInt("interp", 5);
        prefs.end();

        prefs.begin("fb_ui", false);
        prefs.putInt("theme", theme);
        prefs.putInt("icon", icon);
        prefs.putBool("sweep", sweep);
        prefs.putInt("trails", trails);
        prefs.putBool("grid", grid);
        prefs.putBool("startup", startup);
        prefs.putBool("simSwp", simSwp);
        prefs.putInt("tData", tData);
        prefs.putInt("textSize", textSize);
        prefs.putInt("sens", sens);
        prefs.putInt("locAvg", locAvg);
        prefs.putInt("interp", interp);
        prefs.end();

        prefs.begin("radar_zones", true);
        int warnP = prefs.getInt("warnP", ZONE_OFF);
        int deadP = prefs.getInt("deadP", ZONE_OFF);
        int wC_minD = prefs.getInt("wC_minD", 1000);
        int wC_maxD = prefs.getInt("wC_maxD", 3000);
        int wC_minA = prefs.getInt("wC_minA", -30);
        int wC_maxA = prefs.getInt("wC_maxA", 30);
        int dC_minD = prefs.getInt("dC_minD", 0);
        int dC_maxD = prefs.getInt("dC_maxD", 1000);
        int dC_minA = prefs.getInt("dC_minA", -90);
        int dC_maxA = prefs.getInt("dC_maxA", 90);
        int fuzzT = prefs.getInt("fuzzT", 50);
        int histW = prefs.getInt("histW", 10);
        prefs.end();

        prefs.begin("fb_zones", false);
        prefs.putInt("warnP", warnP);
        prefs.putInt("deadP", deadP);
        prefs.putInt("wC_minD", wC_minD);
        prefs.putInt("wC_maxD", wC_maxD);
        prefs.putInt("wC_minA", wC_minA);
        prefs.putInt("wC_maxA", wC_maxA);
        prefs.putInt("dC_minD", dC_minD);
        prefs.putInt("dC_maxD", dC_maxD);
        prefs.putInt("dC_minA", dC_minA);
        prefs.putInt("dC_maxA", dC_maxA);
        prefs.putInt("fuzzT", fuzzT);
        prefs.putInt("histW", histW);
        prefs.end();

        prefs.begin("radar_sys", false);
        prefs.putBool("fb_pend", false);
        prefs.putInt("fb_boots", 0);
        prefs.end();
    }

    void restoreFromFallback() {
        prefs.begin("fb_ui", true);
        int theme = prefs.getInt("theme", 1);
        int icon = prefs.getInt("icon", ICON_SMART);
        bool sweep = prefs.getBool("sweep", true);
        int trails = prefs.getInt("trails", 5);
        bool grid = prefs.getBool("grid", true);
        bool startup = prefs.getBool("startup", true);
        bool simSwp = prefs.getBool("simSwp", false);
        int tData = prefs.getInt("tData", TELEMETRY_OFF);
        int textSize = prefs.getInt("textSize", 1);
        int sens = prefs.getInt("sens", 5);
        int locAvg = prefs.getInt("locAvg", 5);
        int interp = prefs.getInt("interp", 5);
        prefs.end();

        prefs.begin("radar_ui", false);
        prefs.putInt("theme", theme);
        prefs.putInt("icon", icon);
        prefs.putBool("sweep", sweep);
        prefs.putInt("trails", trails);
        prefs.putBool("grid", grid);
        prefs.putBool("startup", startup);
        prefs.putBool("simSwp", simSwp);
        prefs.putInt("tData", tData);
        prefs.putInt("textSize", textSize);
        prefs.putInt("sens", sens);
        prefs.putInt("locAvg", locAvg);
        prefs.putInt("interp", interp);
        prefs.end();

        prefs.begin("fb_zones", true);
        int warnP = prefs.getInt("warnP", ZONE_OFF);
        int deadP = prefs.getInt("deadP", ZONE_OFF);
        int wC_minD = prefs.getInt("wC_minD", 1000);
        int wC_maxD = prefs.getInt("wC_maxD", 3000);
        int wC_minA = prefs.getInt("wC_minA", -30);
        int wC_maxA = prefs.getInt("wC_maxA", 30);
        int dC_minD = prefs.getInt("dC_minD", 0);
        int dC_maxD = prefs.getInt("dC_maxD", 1000);
        int dC_minA = prefs.getInt("dC_minA", -90);
        int dC_maxA = prefs.getInt("dC_maxA", 90);
        int fuzzT = prefs.getInt("fuzzT", 50);
        int histW = prefs.getInt("histW", 10);
        prefs.end();

        prefs.begin("radar_zones", false);
        prefs.putInt("warnP", warnP);
        prefs.putInt("deadP", deadP);
        prefs.putInt("wC_minD", wC_minD);
        prefs.putInt("wC_maxD", wC_maxD);
        prefs.putInt("wC_minA", wC_minA);
        prefs.putInt("wC_maxA", wC_maxA);
        prefs.putInt("dC_minD", dC_minD);
        prefs.putInt("dC_maxD", dC_maxD);
        prefs.putInt("dC_minA", dC_minA);
        prefs.putInt("dC_maxA", dC_maxA);
        prefs.putInt("fuzzT", fuzzT);
        prefs.putInt("histW", histW);
        prefs.end();

        prefs.begin("radar_sys", false);
        prefs.putBool("fb_pend", false);
        prefs.putInt("fb_boots", 0);
        prefs.end();
    }

    bool isFallbackPending() {
        prefs.begin("radar_sys", true);
        bool pending = prefs.getBool("fb_pend", false);
        prefs.end();
        return pending;
    }

    void confirmFallback() {
        prefs.begin("radar_sys", false);
        prefs.putBool("fb_pend", false);
        prefs.putInt("fb_boots", 0);
        prefs.end();
    }

    bool checkFallback() {
        prefs.begin("radar_sys", false);
        bool pending = prefs.getBool("fb_pend", false);
        if (pending) {
            int boots = prefs.getInt("fb_boots", 0);
            boots++;
            prefs.putInt("fb_boots", boots);
            prefs.end();

            if (boots >= 2) {
                Serial.println("Boot loop detected in fallback! Reverting configs...");
                restoreFromFallback();
                delay(1000);
                ESP.restart();
            }
            return true;
        }
        prefs.end();
        return false;
    }
};

#endif
