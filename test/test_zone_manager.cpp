#include <iostream>
#include <cassert>
#include "ZoneManager.h"

void test_isInsideZone_distance() {
    std::cout << "Running test_isInsideZone_distance..." << std::endl;
    ZoneManager zm;
    RadialZone z = {1000, 3000, -90, 90}; // Dist: 1000 to 3000, Angle: full frontal half-plane

    // Test inside
    assert(zm.isInsideZone(0, 2000, z) == true);

    // Test too close
    assert(zm.isInsideZone(0, 500, z) == false);

    // Test too far
    assert(zm.isInsideZone(0, 4000, z) == false);

    // Test exactly on inner boundary
    assert(zm.isInsideZone(0, 1000, z) == true);

    // Test exactly on outer boundary
    assert(zm.isInsideZone(0, 3000, z) == true);

    std::cout << "  ✓ test_isInsideZone_distance passed" << std::endl;
}

void test_isInsideZone_angle() {
    std::cout << "Running test_isInsideZone_angle..." << std::endl;
    ZoneManager zm;
    RadialZone z = {1000, 3000, -30, 30}; // Dist: 1000 to 3000, Angle: -30 to 30 degrees

    // Test straight ahead (0 degrees)
    assert(zm.isInsideZone(0, 2000, z) == true);

    // Test left inside (-20 degrees)
    // x = sin(-20)*2000 = -684, y = cos(-20)*2000 = 1879
    assert(zm.isInsideZone(-684, 1879, z) == true);

    // Test right inside (+20 degrees)
    // x = sin(20)*2000 = 684, y = cos(20)*2000 = 1879
    assert(zm.isInsideZone(684, 1879, z) == true);

    // Test left outside (-40 degrees)
    // x = sin(-40)*2000 = -1285, y = cos(-40)*2000 = 1532
    assert(zm.isInsideZone(-1285, 1532, z) == false);

    // Test right outside (+40 degrees)
    // x = sin(40)*2000 = 1285, y = cos(40)*2000 = 1532
    assert(zm.isInsideZone(1285, 1532, z) == false);

    std::cout << "  ✓ test_isInsideZone_angle passed" << std::endl;
}

void test_isInsideZone_edge_cases() {
    std::cout << "Running test_isInsideZone_edge_cases..." << std::endl;
    ZoneManager zm;
    RadialZone z = {1000, 3000, -30, 30};

    // Test origin (0, 0)
    assert(zm.isInsideZone(0, 0, z) == false);

    // Test negative Y (behind the sensor)
    assert(zm.isInsideZone(0, -2000, z) == false);

    // Test full 360 circle zone
    RadialZone full_circle = {1000, 3000, -180, 180};
    assert(zm.isInsideZone(0, 2000, full_circle) == true);
    assert(zm.isInsideZone(2000, 0, full_circle) == true);
    assert(zm.isInsideZone(-2000, 0, full_circle) == true);
    assert(zm.isInsideZone(0, -2000, full_circle) == true);

    std::cout << "  ✓ test_isInsideZone_edge_cases passed" << std::endl;
}

void test_updateFuzzing() {
    std::cout << "Running test_updateFuzzing..." << std::endl;
    ZoneManager zm;
    zm.setWarnPreset(ZONE_CUSTOM);
    zm.setWarnCustom({1000, 3000, -30, 30});
    zm.setHistoryWindow(5);
    zm.setFuzzingThreshold(60);

    bool targetActive[3] = {false, false, false};
    int16_t targetX[3] = {0, 0, 0};
    int16_t targetY[3] = {0, 0, 0};

    // Frame 1: Target 0 becomes active and is inside the zone
    targetActive[0] = true;
    targetX[0] = 0;
    targetY[0] = 2000;
    zm.updateFuzzing(targetActive, targetX, targetY);

    // Only 1 frame of history, hits=1, checkFrames=1, percent=100 >= 60
    assert(zm.isWarning(0) == true);

    // Frame 2: Target 0 still active and inside
    zm.updateFuzzing(targetActive, targetX, targetY);
    assert(zm.isWarning(0) == true);

    // Frame 3: Target 0 active but outside the zone
    targetX[0] = 0;
    targetY[0] = 4000;
    zm.updateFuzzing(targetActive, targetX, targetY);

    // 3 frames, hits=2, checkFrames=3, percent=66 >= 60
    assert(zm.isWarning(0) == true);

    // Frame 4: Target 0 outside again
    zm.updateFuzzing(targetActive, targetX, targetY);
    // 4 frames, hits=2, checkFrames=4, percent=50 < 60
    assert(zm.isWarning(0) == false);

    // Frame 5: Target 0 outside again
    zm.updateFuzzing(targetActive, targetX, targetY);
    // 5 frames, hits=2, checkFrames=5, percent=40 < 60
    assert(zm.isWarning(0) == false);

    // Test ZONE_OFF resets history and suppresses warnings
    zm.setWarnPreset(ZONE_OFF);
    targetX[0] = 0;
    targetY[0] = 2000; // Back inside
    zm.updateFuzzing(targetActive, targetX, targetY);
    assert(zm.isWarning(0) == false);

    // Switch back to custom, history should have been reset to 0
    zm.setWarnPreset(ZONE_CUSTOM);
    // Need to trigger another update to get a frame of history
    zm.updateFuzzing(targetActive, targetX, targetY);
    assert(zm.isWarning(0) == true);

    // Target becomes inactive, should reset history
    targetActive[0] = false;
    zm.updateFuzzing(targetActive, targetX, targetY);
    assert(zm.isWarning(0) == false);

    std::cout << "  ✓ test_updateFuzzing passed" << std::endl;
}

void test_zone_manager_all() {
    std::cout << "Testing ZoneManager..." << std::endl;
    test_isInsideZone_distance();
    test_isInsideZone_angle();
    test_isInsideZone_edge_cases();
    test_updateFuzzing();
    std::cout << "All ZoneManager tests passed!" << std::endl;
}
