#ifndef ZONE_MANAGER_H
#define ZONE_MANAGER_H

#include <Arduino.h>
#include <math.h>

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

class ZoneManager {
public:
    ZoneManager() {
        warnPreset = ZONE_OFF;
        deadPreset = ZONE_OFF;
        warnCustom = {1000, 3000, -30, 30};
        deadCustom = {0, 1000, -90, 90};
        fuzzingThreshold = 50; // 50%
        historyWindow = 10;    // 10 frames = ~1 second

        for(int i=0; i<3; i++) {
            historyCount[i] = 0;
            for(int j=0; j<30; j++) {
                warnHistory[i][j] = false;
            }
        }
    }

    void setWarnPreset(ZonePreset p) { warnPreset = p; }
    void setDeadPreset(ZonePreset p) { deadPreset = p; }
    void setWarnCustom(RadialZone z) { warnCustom = z; }
    void setDeadCustom(RadialZone z) { deadCustom = z; }
    void setFuzzingThreshold(int percent) { fuzzingThreshold = percent; }
    void setHistoryWindow(int frames) {
        if (frames > 30) frames = 30;
        if (frames < 1) frames = 1;
        historyWindow = frames;
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
        float angleRad = atan2((float)x, (float)y);
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
            float danger = percent / ((float)fuzzingThreshold / 100.0f);
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
