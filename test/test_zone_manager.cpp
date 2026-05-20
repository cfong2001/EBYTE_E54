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

void test_isWarning() {
    std::cout << "Running test_isWarning..." << std::endl;
    ZoneManager zm;
    zm.setWarnPreset(ZONE_CLOSE);
    zm.setFuzzingThreshold(50);
    zm.setHistoryWindow(10);

    // Simulate updating fuzzing with target out of zone (or no targets)
    bool targetActive[3] = {false, false, false};
    int16_t targetX[3] = {0, 0, 0};
    int16_t targetY[3] = {0, 0, 0};

    // Initially should be false (no history)
    assert(zm.isWarning(0) == false);

    // Update fuzzing where target 0 is inside the warning zone for 5 frames
    targetActive[0] = true;
    targetX[0] = 0;
    targetY[0] = 1000; // Inside ZONE_CLOSE (0-2000)

    for(int i = 0; i < 5; i++) {
        zm.updateFuzzing(targetActive, targetX, targetY);
    }

    // History has 5 frames, hits = 5. percent = 100 >= 50, so true
    assert(zm.isWarning(0) == true);

    // Update fuzzing where target 0 is outside the warning zone for 6 frames
    targetY[0] = 3000; // Outside ZONE_CLOSE (0-2000)
    for(int i = 0; i < 6; i++) {
        zm.updateFuzzing(targetActive, targetX, targetY);
    }
    // Now out of 10 frames, hits = 4 (because 1 out of 5 was shifted out, 6 frames false, 4 true)
    // hits = 4. percent = 40. 40 < 50, so false
    assert(zm.isWarning(0) == false);

    // Test that when warnPreset is ZONE_OFF, isWarning is false
    zm.setWarnPreset(ZONE_OFF);
    assert(zm.isWarning(0) == false);

    std::cout << "  ✓ test_isWarning passed" << std::endl;
}

void test_zone_manager_all() {
    std::cout << "Testing ZoneManager..." << std::endl;
    test_isInsideZone_distance();
    test_isInsideZone_angle();
    test_isInsideZone_edge_cases();
    test_isWarning();
    std::cout << "All ZoneManager tests passed!" << std::endl;
}
