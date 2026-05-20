#ifndef BROADCAST_SERVER_H
#define BROADCAST_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "E54_Radar.h"
#include <Preferences.h>

class BroadcastServer {
public:
    BroadcastServer();
    void begin();
    void stop();
    void updateData(const RadarTarget targets[3]);

private:
    AsyncWebServer server;
    RadarTarget currentTargets[3];
    SemaphoreHandle_t bcastMutex;
    bool isRunning;

    void setupRoutes();
};

#endif
