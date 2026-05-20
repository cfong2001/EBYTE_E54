#include <iostream>
#include <cassert>
#include <algorithm>
#include <stdexcept>

#define ESP32
#include "Arduino.h"

class TestLoopExitException : public std::exception {};

#undef vTaskDelay
#define vTaskDelay(x) throw TestLoopExitException()

#define private public
#include "PerformanceMonitor.h"
#undef private

void test_report_timing() {
    std::cout << "Running test_report_timing..." << std::endl;
    PerformanceMonitor pm;
    Serial.clear();
    mock_millis = 1000;

    // Initial report should do nothing as lastMetricsReportTime defaults to 0 and delta is 1000

    pm.incrementFrames(); // 1
    pm.incrementFrames(); // 2
    pm.report();

    assert(Serial.log.size() == 1);
    assert(Serial.log[0].find("[Perf] Frames: 2") != std::string::npos);
    assert(pm.getFrames() == 0);

    // Immediately calling again should do nothing
    Serial.clear();
    pm.incrementFrames();
    pm.report();
    assert(Serial.log.size() == 0);
    assert(pm.getFrames() == 1);

    // After 500ms, still nothing
    mock_millis = 1500;
    pm.report();
    assert(Serial.log.size() == 0);
    assert(pm.getFrames() == 1);

    // After 1000ms total since last report (at 1000ms), it should report
    mock_millis = 2000;
    pm.report();
    assert(Serial.log.size() == 1);
    assert(Serial.log[0].find("[Perf] Frames: 1") != std::string::npos);
    assert(pm.getFrames() == 0);

    std::cout << "  ✓ test_report_timing passed" << std::endl;
}

void test_report_metrics() {
    std::cout << "Running test_report_metrics..." << std::endl;
    PerformanceMonitor pm;
    Serial.clear();
    mock_millis = 1000;

    for(int i=0; i<60; i++) pm.incrementFrames();
    pm.incrementUartErrors();
    pm.incrementUartErrors();
    pm.incrementParseErrors();

    pm.report();

    assert(Serial.log.size() == 1);
    std::string out = Serial.log[0];
    assert(out.find("Frames: 60") != std::string::npos);
    assert(out.find("UART errs: 2") != std::string::npos);
    assert(out.find("Parse errs: 1") != std::string::npos);

    assert(pm.getFrames() == 0);

    std::cout << "  ✓ test_report_metrics passed" << std::endl;
}

void test_task_loop() {
    std::cout << "Running test_task_loop..." << std::endl;
    PerformanceMonitor pm;
    Serial.clear();

    // Set initial state
    pm.lastSystemReportTime = 0;
    mock_millis = 1000; // Not > 5000 yet

    try {
        pm.taskLoop();
    } catch (const TestLoopExitException& e) {
        // Expected
    }

    assert(Serial.log.size() == 0); // shouldn't report yet

    // Now trigger the report
    mock_millis = 5001;

    try {
        pm.taskLoop();
    } catch (const TestLoopExitException& e) {
        // Expected
    }

    // Should have reported
    assert(Serial.log.size() > 0);
    assert(pm.lastSystemReportTime == 5001);

    bool found_heap = false;
    for (const auto& log_msg : Serial.log) {
        if (log_msg.find("Free Heap: 100000 bytes") != std::string::npos) {
            found_heap = true;
            break;
        }
    }
    assert(found_heap);

    std::cout << "  ✓ test_task_loop passed" << std::endl;
}

int main_perf() {
    std::cout << "Testing PerformanceMonitor..." << std::endl;
    test_report_timing();
    test_report_metrics();
    test_task_loop();
    std::cout << "All PerformanceMonitor tests passed!" << std::endl;
    return 0;
}
