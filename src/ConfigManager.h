#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "UIManager.h"
#include "ZoneManager.h"

class ConfigManager {
private:
    Preferences prefs;

public:
    void exportConfig(UIManager& ui) {
        JsonDocument doc;

        // Export UI Settings
        prefs.begin("radar_ui", true);
        doc["theme"] = prefs.getInt("theme", THEME_ALIEN);
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

        // Backup current to fallback
        backupToFallback();

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

        Serial.println("Import successful. Restarting...");
        delay(1000);
        ESP.restart();
        return true;
    }

    void backupToFallback() {
        prefs.begin("radar_ui", true);
        int theme = prefs.getInt("theme", THEME_ALIEN);
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
    }

    void restoreFromFallback() {
        prefs.begin("fb_ui", true);
        int theme = prefs.getInt("theme", THEME_ALIEN);
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
    }

    bool checkFallback() {
        prefs.begin("radar_sys", false);
        bool pending = prefs.getBool("fb_pend", false);
        if (pending) {
            // Clear flag so we don't boot loop if they confirm
            prefs.putBool("fb_pend", false);
        }
        prefs.end();
        return pending;
    }
};

#endif
