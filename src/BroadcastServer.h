
#ifndef BROADCAST_SERVER_H
#define BROADCAST_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "E54_Radar.h"
#include "ZoneManager.h"

class BroadcastServer {
public:
    BroadcastServer();
    void begin();
    void stop();
    void updateData(const RadarTarget targets[3], float velX[3], float velY[3]);
    void updateZones(RadialZone warn, RadialZone dead);

private:
    AsyncWebServer server;
    RadarTarget currentTargets[3];
    float currentVelX[3];
    float currentVelY[3];
    RadialZone currentWarnZone;
    RadialZone currentDeadZone;
    SemaphoreHandle_t bcastMutex;
    bool isRunning;

    void setupRoutes();
};

#endif
