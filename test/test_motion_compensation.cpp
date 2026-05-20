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

int test_motion_compensation_main() {
    std::cout << "Testing MotionCompensation updateFilterState..." << std::endl;
    test_initialization();
    test_steady_state();
    test_high_acceleration();
    std::cout << "All tests passed!" << std::endl;
    return 0;
#include <algorithm>

#include "Arduino.h"
#include "E54_Radar.h"

#define private public
#include "MotionCompensation.h"
#undef private

void test_predictStates_active() {
    std::cout << "Running test_predictStates_active..." << std::endl;
    MotionCompensation mc;
    mc.init();

    // Set up active state
    mc.state[0].active = true;
    mc.state[0].x = 10.0f;
    mc.state[0].y = 20.0f;
    mc.state[0].velX = 2.0f;
    mc.state[0].velY = -3.0f;
    mc.state[0].accX = 1.0f;
    mc.state[0].accY = 0.5f;

    RadarTarget targets[3];
    for(int i = 0; i < 3; i++) {
        targets[i].active = false;
        targets[i].x = 0;
        targets[i].y = 0;
    }
    targets[0].active = true;

    float P_x[3] = {0};
    float P_y[3] = {0};
    float dt = 2.0f;

    mc.predictStates(dt, targets, P_x, P_y);

    // P_x = 10 + 2(2) + 1(4/2) = 16
    // P_y = 20 - 3(2) + 0.5(4/2) = 15
    assert(std::abs(P_x[0] - 16.0f) < 0.001f);
    assert(std::abs(P_y[0] - 15.0f) < 0.001f);

    std::cout << "  ✓ test_predictStates_active passed" << std::endl;
}

void test_predictStates_inactive() {
    std::cout << "Running test_predictStates_inactive..." << std::endl;
    MotionCompensation mc;
    mc.init();

    // Set up inactive state
    mc.state[1].active = false;

    RadarTarget targets[3];
    for(int i = 0; i < 3; i++) {
        targets[i].active = false;
        targets[i].x = 0;
        targets[i].y = 0;
    }
    targets[1].x = 50;
    targets[1].y = 60;

    float P_x[3] = {0};
    float P_y[3] = {0};
    float dt = 2.0f;

    mc.predictStates(dt, targets, P_x, P_y);

    assert(std::abs(P_x[1] - 50.0f) < 0.001f);
    assert(std::abs(P_y[1] - 60.0f) < 0.001f);

    std::cout << "  ✓ test_predictStates_inactive passed" << std::endl;
}

void test_motion_compensation_all() {
    std::cout << "Testing MotionCompensation..." << std::endl;
    test_predictStates_active();
    test_predictStates_inactive();
    std::cout << "All MotionCompensation tests passed!" << std::endl;
}
