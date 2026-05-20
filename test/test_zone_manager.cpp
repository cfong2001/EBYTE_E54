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
void test_getDangerLevel() {
    std::cout << "Running test_getDangerLevel..." << std::endl;
    ZoneManager zm;
    zm.setWarnPreset(ZONE_CUSTOM);
    zm.setWarnCustom({1000, 3000, -90, 90});
    zm.setFuzzingThreshold(50);
    zm.setHistoryWindow(10);

    // Initial check
    assert(zm.getDangerLevel() == 0.0f);

    bool active[3] = {true, false, false};
    int16_t tX[3] = {0, 0, 0};
    int16_t tY_hit[3] = {2000, 0, 0};
    int16_t tY_miss[3] = {5000, 0, 0};

    // Simulate 2 hits and 8 misses for target 0
    // To properly simulate, we need historyCount to reach historyWindow for the math to be easy
    for(int i=0; i<10; i++) {
        bool hit = (i < 2);
        zm.updateFuzzing(active, tX, hit ? tY_hit : tY_miss);
    }

    // hits = 2, window = 10, percent = 0.2, danger = 0.2 / 0.5 = 0.4
    float danger = zm.getDangerLevel();
    assert(danger > 0.39f && danger < 0.41f);

    // Test multiple targets max danger
    // Reset history by disabling them for window duration
    bool inactive[3] = {false, false, false};
    for(int i=0; i<10; i++) {
        zm.updateFuzzing(inactive, tX, tY_miss);
    }

    bool multiActive[3] = {true, true, true};

    // Fill window with specific hits for each
    for (int frame=0; frame<10; frame++) {
        int16_t mY[3] = {5000, 5000, 5000};
        if (frame < 2) mY[0] = 2000; // T0: 2 hits
        if (frame < 4) mY[1] = 2000; // T1: 4 hits
        if (frame < 6) mY[2] = 2000; // T2: 6 hits
        zm.updateFuzzing(multiActive, tX, mY);
    }

    assert(zm.getTargetDangerLevel(0) > 0.39f && zm.getTargetDangerLevel(0) < 0.41f);
    assert(zm.getTargetDangerLevel(1) > 0.79f && zm.getTargetDangerLevel(1) < 0.81f);
    assert(zm.getTargetDangerLevel(2) > 0.99f && zm.getTargetDangerLevel(2) <= 1.01f);

    // Max danger should be clamped to 1.0f from T2
    danger = zm.getDangerLevel();
    assert(danger > 0.99f && danger <= 1.01f);

    // Test that ZONE_OFF forces danger level back to 0.0f
    zm.setWarnPreset(ZONE_OFF);
    assert(zm.getDangerLevel() == 0.0f);

    std::cout << "  ✓ test_getDangerLevel passed" << std::endl;
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
    test_updateFuzzing();
    test_getDangerLevel();
    test_isWarning();
    std::cout << "All ZoneManager tests passed!" << std::endl;
}
