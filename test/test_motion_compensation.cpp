#include <iostream>
#include <cassert>
#include <cmath>
#include "MotionCompensation.h"

// We need an exact match for how we configure and test this.

void test_initialization() {
    std::cout << "Running test_initialization..." << std::endl;
    MotionCompensation mc;
    mc.init();
    assert(mc.state[0].active == false);
    assert(mc.state[1].active == false);
    assert(mc.state[2].active == false);

    // Simulate first data point to verify correct initialization block
    RadarTarget target = {true, 1000, 2000, 10, 50, false};

    mc.updateFilterState(0, 0.1f, 1000.0f, 2000.0f, 1000.0f, 2000.0f, target);

    assert(mc.state[0].active == true);
    assert(mc.state[0].x == 1000.0f);
    assert(mc.state[0].y == 2000.0f);
    assert(mc.state[0].velX == 0.0f);
    assert(mc.state[0].velY == 0.0f);
    assert(mc.state[0].historyCount == 0);

    std::cout << "  ✓ test_initialization passed" << std::endl;
}

void test_steady_state() {
    std::cout << "Running test_steady_state..." << std::endl;
    MotionCompensation mc;
    mc.init();

    RadarTarget target = {true, 1000, 2000, 0, 50, false};

    // Init state
    mc.updateFilterState(0, 0.1f, 1000.0f, 2000.0f, 1000.0f, 2000.0f, target);

    // Simulate 5 frames of stationary data
    for (int i = 0; i < 5; i++) {
        mc.state[0].historyX[mc.state[0].historyHead] = 1000.0f;
        mc.state[0].historyY[mc.state[0].historyHead] = 2000.0f;
        mc.state[0].historyHead = (mc.state[0].historyHead + 1) % MotionCompensation::TargetState::HISTORY_SIZE;
        mc.state[0].historyCount++;
        mc.updateFilterState(0, 0.1f, 1000.0f, 2000.0f, 1000.0f, 2000.0f, target);
    }

    // Position should be exactly 1000, 2000
    assert(std::abs(mc.state[0].x - 1000.0f) < 0.1f);
    assert(std::abs(mc.state[0].y - 2000.0f) < 0.1f);

    // Velocity and accel should be near 0
    assert(std::abs(mc.state[0].velX) < 0.1f);
    assert(std::abs(mc.state[0].velY) < 0.1f);
    assert(std::abs(mc.state[0].accX) < 0.1f);
    assert(std::abs(mc.state[0].accY) < 0.1f);

    std::cout << "  ✓ test_steady_state passed" << std::endl;
}

void test_high_acceleration() {
    std::cout << "Running test_high_acceleration..." << std::endl;
    MotionCompensation mc;
    mc.init();

    RadarTarget target = {true, 1000, 2000, 100, 50, false}; // Speed=100cm/s (1m/s)

    // Initialize state
    mc.updateFilterState(0, 0.1f, 1000.0f, 2000.0f, 1000.0f, 2000.0f, target);

    // Force high acceleration state
    mc.state[0].accX = 3000.0f; // > sqrt(4000000) = 2000 mm/s^2
    mc.state[0].accY = 0.0f;
    mc.state[0].historyCount = 1; // Fake some history
    mc.state[0].historyX[0] = 1000.0f;
    mc.state[0].historyY[0] = 2000.0f;

    // Introduce a sudden jump
    mc.updateFilterState(0, 0.1f, 1100.0f, 2000.0f, 1000.0f, 2000.0f, target);

    // Because of high acceleration, alpha gain should jump and prioritize real-time tracking
    // Without high accel, it would lag behind 1100.0f significantly more
    // With base alpha 0.3, it usually multiplies by 2.5 = 0.75, giving 1000 + 0.75*(1100-1000) = 1075
    assert(mc.state[0].x > 1050.0f);

    std::cout << "  ✓ test_high_acceleration passed" << std::endl;
}

void test_dropout();

int test_motion_compensation_main() {
    std::cout << "Testing MotionCompensation updateFilterState..." << std::endl;
    test_initialization();
    test_steady_state();
    test_high_acceleration();
    test_dropout();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}

void test_dropout() {
    std::cout << "Running test_dropout..." << std::endl;
    MotionCompensation mc;
    mc.init();

    RadarTarget targets[3];
    RadarTarget compensated[3];

    // Initialize with valid target
    targets[0] = {true, 1000, 2000, 100, 50, false};
    targets[1] = {false, 0, 0, 0, 0, false};
    targets[2] = {false, 0, 0, 0, 0, false};

    mc.process(0.1f, targets, compensated);
    assert(compensated[0].active == true);
    assert(compensated[0].isCoasting == false);

    // Give it a velocity by updating again
    targets[0] = {true, 1000, 2100, 100, 50, false}; // Moving away, Y increases
    mc.process(0.1f, targets, compensated);

    // We can't access mc.state without macro magic here, but we can check coasting logic via outputs

    // Simulate dropout
    targets[0] = {false, 0, 0, 0, 0, false};
    mc.process(0.1f, targets, compensated);

    assert(compensated[0].active == true); // Should still be active
    assert(compensated[0].isCoasting == true); // Should be coasting

    // Simulate permanent drop after maxFramesLost (10)
    for (int i = 0; i < 10; i++) {
        mc.process(0.1f, targets, compensated);
    }

    assert(compensated[0].active == false);
    assert(compensated[0].isCoasting == false);

    std::cout << "  ✓ test_dropout passed" << std::endl;
}
