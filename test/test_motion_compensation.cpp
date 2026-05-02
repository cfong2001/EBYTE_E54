#include <iostream>
#include <cassert>
#include "MotionCompensation.h"

void test_init() {
    std::cout << "Running test_init..." << std::endl;
    MotionCompensation mc;

    // Initialize
    mc.init();

    // Set some data via process to change internal state
    RadarTarget targets[3] = {0};
    RadarTarget compensated[3] = {0};

    targets[0].active = true;
    targets[0].x = 1000;
    targets[0].y = 2000;
    targets[0].speed = 0;

    // Process a few times to let the filter initialize and track
    for(int i=0; i<5; i++) {
        mc.process(targets, compensated);
    }

    // Verify state is non-zero/active
    assert(mc.isTargetActive(0) == true);
    // After several frames at (1000, 2000), vel and acc should be small but mc should be active

    std::cout << "  State populated. Resetting..." << std::endl;

    // Call init() to test reset
    mc.init();

    // Verify all states are reset
    for(int i=0; i<3; i++) {
        assert(mc.isTargetActive(i) == false);
        assert(mc.getTargetVelX(i) == 0.0f);
        assert(mc.getTargetVelY(i) == 0.0f);
        assert(mc.getTargetAccX(i) == 0.0f);
        assert(mc.getTargetAccY(i) == 0.0f);
    }
    assert(mc.isAnchorValid() == false);

    std::cout << "  ✓ test_init passed!" << std::endl;
}

int main() {
    try {
        test_init();
        std::cout << "All tests passed successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
