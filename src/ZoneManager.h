#ifndef ZONE_MANAGER_H
#define ZONE_MANAGER_H

#include <Arduino.h>
#include <math.h>
#include <Preferences.h>

enum ZonePreset {
    ZONE_OFF,
    ZONE_CLOSE,
    ZONE_MEDIUM,
    ZONE_FAR,
    ZONE_CUSTOM
};

struct RadialZone {
    int minDist;   // mm
    int maxDist;   // mm
    int minAngle;  // degrees (-90 to 90, 0 is straight ahead)
    int maxAngle;  // degrees
};

#include <Preferences.h>

class ZoneManager {
public:
    Preferences preferences;

    ZoneManager() {
        warnPreset = ZONE_OFF;
        deadPreset = ZONE_OFF;
        warnCustom = {1000, 3000, -30, 30};
        deadCustom = {0, 1000, -90, 90};
        fuzzingThreshold = 50;
        historyWindow = 10;

        for(int i=0; i<3; i++) {
            historyCount[i] = 0;
            for(int j=0; j<30; j++) warnHistory[i][j] = false;
        }
    }




    struct IntPrefMapping {
        const char* key;
        int* valuePtr;
        int defaultVal;
    };

    void loadSettings() {
        preferences.begin("radar_zones", false);

        int tempWarnP, tempDeadP;
        IntPrefMapping intMappings[] = {
            {"warnP", &tempWarnP, ZONE_OFF},
            {"deadP", &tempDeadP, ZONE_OFF},
            {"wC_minD", &warnCustom.minDist, 1000},
            {"wC_maxD", &warnCustom.maxDist, 3000},
            {"wC_minA", &warnCustom.minAngle, -30},
            {"wC_maxA", &warnCustom.maxAngle, 30},
            {"dC_minD", &deadCustom.minDist, 0},
            {"dC_maxD", &deadCustom.maxDist, 1000},
            {"dC_minA", &deadCustom.minAngle, -90},
            {"dC_maxA", &deadCustom.maxAngle, 90},
            {"fuzzT", &fuzzingThreshold, 50},
            {"histW", &historyWindow, 10}
        };

        for (const auto& mapping : intMappings) {
            *(mapping.valuePtr) = preferences.getInt(mapping.key, mapping.defaultVal);
        }

        warnPreset = (ZonePreset)tempWarnP;
        deadPreset = (ZonePreset)tempDeadP;

        preferences.end();
    }

    void saveSettings() {
        preferences.begin("radar_zones", false);

        int tempWarnP = warnPreset;
        int tempDeadP = deadPreset;

        IntPrefMapping intMappings[] = {
            {"warnP", &tempWarnP, 0},
            {"deadP", &tempDeadP, 0},
            {"wC_minD", &warnCustom.minDist, 0},
            {"wC_maxD", &warnCustom.maxDist, 0},
            {"wC_minA", &warnCustom.minAngle, 0},
            {"wC_maxA", &warnCustom.maxAngle, 0},
            {"dC_minD", &deadCustom.minDist, 0},
            {"dC_maxD", &deadCustom.maxDist, 0},
            {"dC_minA", &deadCustom.minAngle, 0},
            {"dC_maxA", &deadCustom.maxAngle, 0},
            {"fuzzT", &fuzzingThreshold, 0},
            {"histW", &historyWindow, 0}
        };

        for (const auto& mapping : intMappings) {
            preferences.putInt(mapping.key, *(mapping.valuePtr));
        }

        preferences.end();
    }


    void setWarnPreset(ZonePreset p) { warnPreset = p; saveSettings(); }
    void setDeadPreset(ZonePreset p) { deadPreset = p; saveSettings(); }
    void setWarnCustom(RadialZone z) { warnCustom = z; saveSettings(); }
    void setDeadCustom(RadialZone z) { deadCustom = z; saveSettings(); }
    void setFuzzingThreshold(int percent) { fuzzingThreshold = percent; saveSettings(); }
    void setHistoryWindow(int frames) {
        if (frames > 30) frames = 30;
        if (frames < 1) frames = 1;
        historyWindow = frames;
        saveSettings();
    }



    ZonePreset getWarnPreset() const { return warnPreset; }
    ZonePreset getDeadPreset() const { return deadPreset; }
    RadialZone getWarnCustom() const { return warnCustom; }
    RadialZone getDeadCustom() const { return deadCustom; }
    int getFuzzingThreshold() const { return fuzzingThreshold; }
    int getHistoryWindow() const { return historyWindow; }

    RadialZone getActiveWarnZone() const { return getZoneFromPreset(warnPreset, warnCustom); }
    RadialZone getActiveDeadZone() const { return getZoneFromPreset(deadPreset, deadCustom); }

    bool isInsideZone(int16_t x, int16_t y, RadialZone z) const {
        if (x == 0 && y == 0) return false;
        long distSq = (long)x*x + (long)y*y;
        long minDistSq = (long)z.minDist * z.minDist;
        long maxDistSq = (long)z.maxDist * z.maxDist;
        if (distSq < minDistSq || distSq > maxDistSq) return false;

        if (z.minAngle <= -90 && z.maxAngle >= 90) return true;

        float angleRad = atan2f((float)x, (float)y);
        int angleDeg = (int)(angleRad * 180.0f / PI);
        if (angleDeg < z.minAngle || angleDeg > z.maxAngle) return false;
        return true;
    }

    bool isDead(int16_t x, int16_t y) const {
        if (deadPreset == ZONE_OFF) return false;
        return isInsideZone(x, y, getActiveDeadZone());
    }

    void updateFuzzing(bool* targetActive, int16_t* targetX, int16_t* targetY) {
        RadialZone z = getActiveWarnZone();
        bool warnEnabled = (warnPreset != ZONE_OFF);
        for (int i=0; i<3; i++) {
            for (int h=29; h>0; h--) {
                warnHistory[i][h] = warnHistory[i][h-1];
            }
            if (targetActive[i] && warnEnabled) {
                warnHistory[i][0] = isInsideZone(targetX[i], targetY[i], z);
                if (historyCount[i] < historyWindow) historyCount[i]++;
            } else {
                warnHistory[i][0] = false;
                historyCount[i] = 0;
            }
        }
    }

    bool isWarning(int targetId) const {
        if (warnPreset == ZONE_OFF) return false;
        if (historyCount[targetId] == 0) return false;
        int hits = 0;
        int checkFrames = historyWindow;
        if (historyCount[targetId] < historyWindow) checkFrames = historyCount[targetId];
        for (int h=0; h<checkFrames; h++) {
            if (warnHistory[targetId][h]) hits++;
        }
        int percent = (hits * 100) / checkFrames;
        return (percent >= fuzzingThreshold);
    }

    float getTargetDangerLevel(int targetId) const {
        if (warnPreset == ZONE_OFF) return 0.0f;
        if (historyCount[targetId] == 0) return 0.0f;

        int hits = 0;
        int checkFrames = historyWindow;
        if (historyCount[targetId] < historyWindow) checkFrames = historyCount[targetId];

        for (int h=0; h<checkFrames; h++) {
            if (warnHistory[targetId][h]) hits++;
        }

        float percent = (float)hits / (float)checkFrames;
        float danger = 1.0f;

        if (fuzzingThreshold > 0) {
            danger = percent / ((float)fuzzingThreshold / 100.0f);
        }

        if (danger > 1.0f) danger = 1.0f;
        return danger;
    }

    float getDangerLevel() const {
        if (warnPreset == ZONE_OFF) return 0.0f;
        float maxDanger = 0.0f;
        for (int i=0; i<3; i++) {
            float danger = getTargetDangerLevel(i);
            if (danger > maxDanger) maxDanger = danger;
        }
        return maxDanger;
    }

private:
    ZonePreset warnPreset;
    ZonePreset deadPreset;
    RadialZone warnCustom;
    RadialZone deadCustom;
    int fuzzingThreshold;
    int historyWindow;
    bool warnHistory[3][30];
    int historyCount[3];

    RadialZone getZoneFromPreset(ZonePreset p, RadialZone custom) const {
        switch(p) {
            case ZONE_CLOSE:  return {0, 2000, -90, 90};
            case ZONE_MEDIUM: return {2000, 4000, -90, 90};
            case ZONE_FAR:    return {4000, 6000, -90, 90};
            case ZONE_CUSTOM: return custom;
            default:          return {0, 0, 0, 0};
        }
    }
};

#endif
