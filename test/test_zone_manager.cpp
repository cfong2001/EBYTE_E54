#include <iostream>
#include <cassert>
#define private public
#include "ZoneManager.h"
#undef private

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
    RadialZone customZone = {1000, 3000, -90, 90};
    zm.setWarnCustom(customZone);
    zm.setHistoryWindow(5);

    // Check initial state
    for(int i=0; i<3; i++) {
        assert(zm.historyCount[i] == 0);
    }

    // Frame 1
    bool targetActive1[3] = {true, true, false};
    int16_t targetX1[3] = {0, 0, 0};
    int16_t targetY1[3] = {2000, 500, 0}; // target 0 inside, target 1 outside (too close), target 2 inactive

    zm.updateFuzzing(targetActive1, targetX1, targetY1);

    assert(zm.historyCount[0] == 1);
    assert(zm.warnHistory[0][0] == true);

    assert(zm.historyCount[1] == 1);
    assert(zm.warnHistory[1][0] == false);

    assert(zm.historyCount[2] == 0);
    assert(zm.warnHistory[2][0] == false);

    // Frame 2
    bool targetActive2[3] = {true, true, false};
    int16_t targetX2[3] = {0, 0, 0};
    int16_t targetY2[3] = {500, 2000, 0}; // target 0 outside, target 1 inside, target 2 inactive

    zm.updateFuzzing(targetActive2, targetX2, targetY2);

    assert(zm.historyCount[0] == 2);
    assert(zm.warnHistory[0][0] == false);
    assert(zm.warnHistory[0][1] == true); // shifted

    assert(zm.historyCount[1] == 2);
    assert(zm.warnHistory[1][0] == true);
    assert(zm.warnHistory[1][1] == false); // shifted

    // Frame 3 (Target 0 becomes inactive)
    bool targetActive3[3] = {false, true, false};
    int16_t targetX3[3] = {0, 0, 0};
    int16_t targetY3[3] = {0, 2000, 0};

    zm.updateFuzzing(targetActive3, targetX3, targetY3);

    assert(zm.historyCount[0] == 0); // Reset
    assert(zm.warnHistory[0][0] == false); // Reset for this frame, shifted past is ignored since count is 0

    assert(zm.historyCount[1] == 3);
    assert(zm.warnHistory[1][0] == true);
    assert(zm.warnHistory[1][1] == true);
    assert(zm.warnHistory[1][2] == false);

    // Test warnPreset == ZONE_OFF
    zm.setWarnPreset(ZONE_OFF);
    bool targetActive4[3] = {true, false, false};
    int16_t targetX4[3] = {0, 0, 0};
    int16_t targetY4[3] = {2000, 0, 0};

    zm.updateFuzzing(targetActive4, targetX4, targetY4);
    // When warnEnabled is false (ZONE_OFF), it falls into the else branch:
    // targetActive[i] && warnEnabled is false
    assert(zm.historyCount[0] == 0);
    assert(zm.warnHistory[0][0] == false);

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
