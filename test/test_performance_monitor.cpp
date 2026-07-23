#include <iostream>
#include <cassert>
#include <algorithm>
#define private public
#include "PerformanceMonitor.h"
#undef private

void test_report_timing() {
    std::cout << "Running test_report_timing..." << std::endl;
    PerformanceMonitor pm;
    Serial.clear();
    mock_millis = 1000;

    // Initial report should do nothing as lastMetricsReportTime defaults to 0 and delta is 1000

    pm.framesThisSecond++; // 1
    pm.framesThisSecond++; // 2
    pm.report();

    assert(Serial.log.size() == 1);
    assert(Serial.log[0].find("[Perf] Frames: 2") != std::string::npos);
    assert(pm.framesThisSecond == 0);

    // Immediately calling again should do nothing
    Serial.clear();
    pm.framesThisSecond++;
    pm.report();
    assert(Serial.log.size() == 0);
    assert(pm.framesThisSecond == 1);

    // After 500ms, still nothing
    mock_millis = 1500;
    pm.report();
    assert(Serial.log.size() == 0);
    assert(pm.framesThisSecond == 1);

    // After 1000ms total since last report (at 1000ms), it should report
    mock_millis = 2000;
    pm.report();
    assert(Serial.log.size() == 1);
    assert(Serial.log[0].find("[Perf] Frames: 1") != std::string::npos);
    assert(pm.framesThisSecond == 0);

    std::cout << "  ✓ test_report_timing passed" << std::endl;
}

void test_report_metrics() {
    std::cout << "Running test_report_metrics..." << std::endl;
    PerformanceMonitor pm;
    Serial.clear();
    mock_millis = 1000;

    for(int i=0; i<60; i++) pm.framesThisSecond++;
    pm.uartErrorsThisSecond++;
    pm.uartErrorsThisSecond++;
    pm.parseErrorsThisSecond++;

    pm.report();

    assert(Serial.log.size() == 1);
    std::string out = Serial.log[0];
    assert(out.find("Frames: 60") != std::string::npos);
    assert(out.find("UART errs: 2") != std::string::npos);
    assert(out.find("Parse errs: 1") != std::string::npos);

    assert(pm.framesThisSecond == 0);

    std::cout << "  ✓ test_report_metrics passed" << std::endl;
}

int main_perf() {
    std::cout << "Testing PerformanceMonitor..." << std::endl;
    test_report_timing();
    test_report_metrics();
    std::cout << "All PerformanceMonitor tests passed!" << std::endl;
    return 0;
}
