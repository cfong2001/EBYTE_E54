#include <cmath>
#include <iostream>
#include <cassert>
#include "ZoneManager.h"

void test_isInsideZone_distance() {
    std::cout << "Running test_isInsideZone_distance..." << std::endl;
    ZoneManager zm;
    RadialZone z = {1000, 3000, -90, 90, 0.0f, 0.0f};
    if (z.minAngle > -90 && z.maxAngle < 90) { z.minTan = tanf(z.minAngle / 57.2957795f); z.maxTan = tanf(z.maxAngle / 57.2957795f); } // Dist: 1000 to 3000, Angle: full frontal half-plane

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
    RadialZone z = {1000, 3000, -30, 30, 0.0f, 0.0f};
    if (z.minAngle > -90 && z.maxAngle < 90) { z.minTan = tanf(z.minAngle / 57.2957795f); z.maxTan = tanf(z.maxAngle / 57.2957795f); } // Dist: 1000 to 3000, Angle: -30 to 30 degrees

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
    RadialZone z = {1000, 3000, -30, 30, 0.0f, 0.0f};
    if (z.minAngle > -90 && z.maxAngle < 90) { z.minTan = tanf(z.minAngle / 57.2957795f); z.maxTan = tanf(z.maxAngle / 57.2957795f); }

    // Test origin (0, 0)
    assert(zm.isInsideZone(0, 0, z) == false);

    // Test negative Y (behind the sensor)
    assert(zm.isInsideZone(0, -2000, z) == false);

    // Test full 360 circle zone
    RadialZone full_circle = {1000, 3000, -180, 180, 0.0f, 0.0f};
    if (full_circle.minAngle > -90 && full_circle.maxAngle < 90) { full_circle.minTan = tanf(full_circle.minAngle / 57.2957795f); full_circle.maxTan = tanf(full_circle.maxAngle / 57.2957795f); }
    assert(zm.isInsideZone(0, 2000, full_circle) == true);
    assert(zm.isInsideZone(2000, 0, full_circle) == true);
    assert(zm.isInsideZone(-2000, 0, full_circle) == true);
    assert(zm.isInsideZone(0, -2000, full_circle) == true);

    std::cout << "  ✓ test_isInsideZone_edge_cases passed" << std::endl;
}

void test_zone_manager_all() {
    std::cout << "Testing ZoneManager..." << std::endl;
    test_isInsideZone_distance();
    test_isInsideZone_angle();
    test_isInsideZone_edge_cases();
    std::cout << "All ZoneManager tests passed!" << std::endl;
}
