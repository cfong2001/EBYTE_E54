import re

# Update BroadcastServer.h
with open('src/BroadcastServer.h', 'r') as f:
    h_content = f.read()

h_patch = """
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
    void updateData(const RadarTarget targets[3]);
    void updateZones(RadialZone warn, RadialZone dead);

private:
    AsyncWebServer server;
    RadarTarget currentTargets[3];
    RadialZone currentWarnZone;
    RadialZone currentDeadZone;
    SemaphoreHandle_t bcastMutex;
    bool isRunning;

    void setupRoutes();
};

#endif
"""

with open('src/BroadcastServer.h', 'w') as f:
    f.write(h_patch)

# Fix BroadcastServer.cpp else error
with open('src/BroadcastServer.cpp', 'r') as f:
    cpp_content = f.read()

# remove extra 'else {' that was injected
cpp_content = cpp_content.replace('else { else {', 'else {')
with open('src/BroadcastServer.cpp', 'w') as f:
    f.write(cpp_content)
