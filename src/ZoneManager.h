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
    float minTan;  // precomputed tangent of minAngle (for optimization)
    float maxTan;  // precomputed tangent of maxAngle (for optimization)
};

#include <Preferences.h>

class ZoneManager {
public:
    Preferences preferences;

    ZoneManager() {
        warnPreset = ZONE_OFF;
        deadPreset = ZONE_OFF;
        warnCustom = {1000, 3000, -30, 30, 0.0f, 0.0f};
        deadCustom = {0, 1000, -90, 90, 0.0f, 0.0f};
        updateTangents(warnCustom);
        updateTangents(deadCustom);
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
        updateTangents(warnCustom);
        updateTangents(deadCustom);
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

    void setWarnPreset(ZonePreset p) { warnPreset = p; flagDirty(); }
    void setDeadPreset(ZonePreset p) { deadPreset = p; flagDirty(); }
    void setWarnCustom(RadialZone z) { warnCustom = z; updateTangents(warnCustom); flagDirty(); }
    void setDeadCustom(RadialZone z) { deadCustom = z; updateTangents(deadCustom); flagDirty(); }
    void setFuzzingThreshold(int percent) { fuzzingThreshold = percent; flagDirty(); }
    void setHistoryWindow(int frames) {
        if (frames > 30) frames = 30;
        if (frames < 1) frames = 1;
        historyWindow = frames;
        flagDirty();
    }



    ZonePreset getWarnPreset() { return warnPreset; }
    ZonePreset getDeadPreset() { return deadPreset; }
    RadialZone getWarnCustom() { return warnCustom; }
    RadialZone getDeadCustom() { return deadCustom; }
    int getFuzzingThreshold() { return fuzzingThreshold; }
    int getHistoryWindow() { return historyWindow; }

    RadialZone getActiveWarnZone() { return getZoneFromPreset(warnPreset, warnCustom); }
    RadialZone getActiveDeadZone() { return getZoneFromPreset(deadPreset, deadCustom); }

    bool isInsideZone(int16_t x, int16_t y, RadialZone z) const {
        if (x == 0 && y == 0) return false;
        long distSq = (long)x*x + (long)y*y;
        long minDistSq = (long)z.minDist * z.minDist;
        long maxDistSq = (long)z.maxDist * z.maxDist;
        if (distSq < minDistSq || distSq > maxDistSq) return false;

        // Fast path for full 360 zone
        if (z.minAngle <= -180 && z.maxAngle >= 180) return true;

        // Fast tangent check when target is directly in front (y > 0)
        // Avoids expensive atan2f() function call by using tangent ratios
        if (y > 0 && z.minAngle > -90 && z.maxAngle < 90) {
            float y_f = (float)y;
            float x_f = (float)x;
            if (x_f < z.minTan * y_f || x_f > z.maxTan * y_f) return false;
        } else {
            // Fallback for edge cases, behind sensor, or 180+ FOV
            float angle = atan2f((float)x, (float)y) * 57.2957795f;
            if (angle < z.minAngle || angle > z.maxAngle) return false;
        }

        return true;
    }

    bool runSelfTest() {
        ZoneManager testZm;
        RadialZone testZone = {1000, 3000, -90, 90, 0.0f, 0.0f};
        if (!testZm.isInsideZone(0, 2000, testZone) || testZm.isInsideZone(0, 500, testZone)) {
            return false;
        }
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

    float getTargetDangerLevel(int targetId) {
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
            danger = percent / ((float)fuzzingThreshold * 0.01f);
        }

        if (danger > 1.0f) danger = 1.0f;
        return danger;
    }

    float getDangerLevel() {
        if (warnPreset == ZONE_OFF) return 0.0f;
        float maxDanger = 0.0f;
        for (int i=0; i<3; i++) {
            float danger = getTargetDangerLevel(i);
            if (danger > maxDanger) maxDanger = danger;
        }
        return maxDanger;
    }


    bool isDirty = false;
    unsigned long lastDirtyTime = 0;

    void flagDirty() {
        isDirty = true;
        lastDirtyTime = millis();
    }

    void handleDeferredSave() {
        if (isDirty && (millis() - lastDirtyTime > 2000)) {
            saveSettings();
            isDirty = false;
        }
    }

private:
    void updateTangents(RadialZone& z) {
        if (z.minAngle > -90 && z.maxAngle < 90) {
            z.minTan = tanf(z.minAngle / 57.2957795f);
            z.maxTan = tanf(z.maxAngle / 57.2957795f);
        }
    }

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
            case ZONE_CLOSE:  return {0, 2000, -90, 90, 0.0f, 0.0f};
            case ZONE_MEDIUM: return {2000, 4000, -90, 90, 0.0f, 0.0f};
            case ZONE_FAR:    return {4000, 6000, -90, 90, 0.0f, 0.0f};
            case ZONE_CUSTOM: return custom;
            default:          return {0, 0, 0, 0, 0.0f, 0.0f};
        }
    }
};

#endif
