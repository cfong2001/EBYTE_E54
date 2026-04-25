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

    void loadSettings() {
        preferences.begin("radar_zones", false);
        warnPreset = (ZonePreset)preferences.getInt("warnP", ZONE_OFF);
        deadPreset = (ZonePreset)preferences.getInt("deadP", ZONE_OFF);
        warnCustom.minDist = preferences.getInt("wC_minD", 1000);
        warnCustom.maxDist = preferences.getInt("wC_maxD", 3000);
        warnCustom.minAngle = preferences.getInt("wC_minA", -30);
        warnCustom.maxAngle = preferences.getInt("wC_maxA", 30);
        deadCustom.minDist = preferences.getInt("dC_minD", 0);
        deadCustom.maxDist = preferences.getInt("dC_maxD", 1000);
        deadCustom.minAngle = preferences.getInt("dC_minA", -90);
        deadCustom.maxAngle = preferences.getInt("dC_maxA", 90);
        fuzzingThreshold = preferences.getInt("fuzzT", 50);
        historyWindow = preferences.getInt("histW", 10);
        preferences.end();
    }

    void saveSettings() {
        preferences.begin("radar_zones", false);
        preferences.putInt("warnP", warnPreset);
        preferences.putInt("deadP", deadPreset);
        preferences.putInt("wC_minD", warnCustom.minDist);
        preferences.putInt("wC_maxD", warnCustom.maxDist);
        preferences.putInt("wC_minA", warnCustom.minAngle);
        preferences.putInt("wC_maxA", warnCustom.maxAngle);
        preferences.putInt("dC_minD", deadCustom.minDist);
        preferences.putInt("dC_maxD", deadCustom.maxDist);
        preferences.putInt("dC_minA", deadCustom.minAngle);
        preferences.putInt("dC_maxA", deadCustom.maxAngle);
        preferences.putInt("fuzzT", fuzzingThreshold);
        preferences.putInt("histW", historyWindow);
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



    ZonePreset getWarnPreset() { return warnPreset; }
    ZonePreset getDeadPreset() { return deadPreset; }
    RadialZone getWarnCustom() { return warnCustom; }
    RadialZone getDeadCustom() { return deadCustom; }
    int getFuzzingThreshold() { return fuzzingThreshold; }
    int getHistoryWindow() { return historyWindow; }

    RadialZone getActiveWarnZone() { return getZoneFromPreset(warnPreset, warnCustom); }
    RadialZone getActiveDeadZone() { return getZoneFromPreset(deadPreset, deadCustom); }

    bool isInsideZone(int16_t x, int16_t y, RadialZone z) {
        if (x == 0 && y == 0) return false;
        long distSq = (long)x*x + (long)y*y;
        long minDistSq = (long)z.minDist * z.minDist;
        long maxDistSq = (long)z.maxDist * z.maxDist;
        if (distSq < minDistSq || distSq > maxDistSq) return false;
        float angleRad = atan2f((float)x, (float)y);
        int angleDeg = (int)(angleRad * 180.0f / PI);
        if (angleDeg < z.minAngle || angleDeg > z.maxAngle) return false;
        return true;
    }

    bool isDead(int16_t x, int16_t y) {
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

    bool isWarning(int targetId) {
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

    float getDangerLevel() {
        if (warnPreset == ZONE_OFF) return 0.0f;
        float maxDanger = 0.0f;
        for (int i=0; i<3; i++) {
            if (historyCount[i] == 0) continue;
            int hits = 0;
            int checkFrames = historyWindow;
            if (historyCount[i] < historyWindow) checkFrames = historyCount[i];
            for (int h=0; h<checkFrames; h++) {
                if (warnHistory[i][h]) hits++;
            }
            float percent = (float)hits / (float)checkFrames;
            float danger = 1.0f;
            if (fuzzingThreshold > 0) {
                danger = percent / ((float)fuzzingThreshold / 100.0f);
            }
            if (danger > 1.0f) danger = 1.0f;
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

    RadialZone getZoneFromPreset(ZonePreset p, RadialZone custom) {
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
