#include <iostream>
#include <cassert>
#include "Arduino.h"

// Macro trick to access private members for testing purposes only
#define private public
#include "BroadcastServer.h"
#undef private

MockWiFi WiFi;

void test_broadcast_server_updateData() {
    std::cout << "Running test_broadcast_server_updateData..." << std::endl;

    BroadcastServer server;

    // Create test targets
    RadarTarget targets[3];
    for (int i = 0; i < 3; i++) {
        targets[i].active = true;
        targets[i].x = 100 * (i + 1);
        targets[i].y = 200 * (i + 1);
        targets[i].speed = 10 * (i + 1);
        targets[i].resolution = 1 * (i + 1);
        targets[i].isCoasting = false;
    }

    // Update data
    server.updateData(targets);

    // Verify the data was copied correctly to the internal currentTargets array
    for (int i = 0; i < 3; i++) {
        assert(server.currentTargets[i].active == targets[i].active);
        assert(server.currentTargets[i].x == targets[i].x);
        assert(server.currentTargets[i].y == targets[i].y);
        assert(server.currentTargets[i].speed == targets[i].speed);
        assert(server.currentTargets[i].resolution == targets[i].resolution);
        assert(server.currentTargets[i].isCoasting == targets[i].isCoasting);
    }

    // Test that passing empty targets overwrites the old targets correctly
    RadarTarget emptyTargets[3];
    for (int i = 0; i < 3; i++) {
        emptyTargets[i].active = false;
        emptyTargets[i].x = 0;
        emptyTargets[i].y = 0;
        emptyTargets[i].speed = 0;
        emptyTargets[i].resolution = 0;
        emptyTargets[i].isCoasting = false;
    }
    server.updateData(emptyTargets);

    for (int i = 0; i < 3; i++) {
        assert(server.currentTargets[i].active == false);
        assert(server.currentTargets[i].x == 0);
        assert(server.currentTargets[i].y == 0);
        assert(server.currentTargets[i].speed == 0);
        assert(server.currentTargets[i].resolution == 0);
    }

    std::cout << "  ✓ test_broadcast_server_updateData passed" << std::endl;
}
