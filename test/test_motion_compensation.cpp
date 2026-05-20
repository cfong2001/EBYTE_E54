#include <iostream>
#include <cassert>
#include <cmath>
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
