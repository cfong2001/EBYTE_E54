#include <iostream>
#include <cmath>
#include <cassert>
#include "E54_Radar.h"

void test_radar_parse_payload_positive_negative() {
    std::cout << "Running test_radar_parse_payload_positive_negative..." << std::endl;

    HardwareSerial mockSerial;
    E54_Radar radar(mockSerial);

    // Construct a mock payload for positive and negative values
    // Payload length: 24 bytes (3 targets * 8 bytes/target)

    // Target 1: x = 1500 mm (positive), y = -800 mm (negative), speed = 25 cm/s (positive), res = 5 mm
    // x = 1500 (0x05DC). Positive -> 0x8000 | 0x05DC = 0x85DC -> buf = 0xDC, 0x85
    // y = -800 (0x0320). Negative -> 0x0000 | 0x0320 = 0x0320 -> buf = 0x20, 0x03
    // speed = 25 (0x0019). Positive -> 0x8000 | 0x0019 = 0x8019 -> buf = 0x19, 0x80
    // res = 5 (0x0005) -> buf = 0x05, 0x00

    // Target 2: x = -100 mm (negative), y = 3000 mm (positive), speed = -10 cm/s (negative), res = 2 mm
    // x = -100 (0x0064) -> 0x0000 | 0x0064 = 0x0064 -> buf = 0x64, 0x00
    // y = 3000 (0x0BB8) -> 0x8000 | 0x0BB8 = 0x8BB8 -> buf = 0xB8, 0x8B
    // speed = -10 (0x000A) -> 0x0000 | 0x000A = 0x000A -> buf = 0x0A, 0x00
    // res = 2 (0x0002) -> buf = 0x02, 0x00

    // Target 3: empty (all zeros)

    mockSerial.write(0xAA);
    mockSerial.write(0xFF);
    mockSerial.write(0x03);
    mockSerial.write(0x00);

    // Target 1
    mockSerial.write(0xDC); mockSerial.write(0x85);
    mockSerial.write(0x20); mockSerial.write(0x03);
    mockSerial.write(0x19); mockSerial.write(0x80);
    mockSerial.write(0x05); mockSerial.write(0x00);

    // Target 2
    mockSerial.write(0x64); mockSerial.write(0x00);
    mockSerial.write(0xB8); mockSerial.write(0x8B);
    mockSerial.write(0x0A); mockSerial.write(0x00);
    mockSerial.write(0x02); mockSerial.write(0x00);

    // Target 3
    for(int i=0; i<8; i++) mockSerial.write(0x00);

    mockSerial.write(0x55);
    mockSerial.write(0xCC);

    bool updated = radar.update();
    assert(updated == true);

    // Verify Target 1
    assert(radar.targets[0].active == true);
    assert(radar.targets[0].x == 1500);
    assert(radar.targets[0].y == -800);
    assert(radar.targets[0].speed == 25);
    assert(radar.targets[0].resolution == 5);

    // Verify Target 2
    assert(radar.targets[1].active == true);
    assert(radar.targets[1].x == -100);
    assert(radar.targets[1].y == 3000);
    assert(radar.targets[1].speed == -10);
    assert(radar.targets[1].resolution == 2);

    // Verify Target 3
    assert(radar.targets[2].active == false);
    // Since inactive target variables are not reset, their x/y will be whatever was there before (which is uninitialized if we don't set it in struct init).
    // Let's just assert it is inactive.

    std::cout << "  ✓ test_radar_parse_payload_positive_negative passed" << std::endl;
}

void test_radar_parse_payload_empty() {
    std::cout << "Running test_radar_parse_payload_empty..." << std::endl;

    HardwareSerial mockSerial;
    E54_Radar radar(mockSerial);

    // Construct a mock payload for empty targets
    mockSerial.write(0xAA);
    mockSerial.write(0xFF);
    mockSerial.write(0x03);
    mockSerial.write(0x00);

    // Target 1-3
    for(int i=0; i<24; i++) mockSerial.write(0x00);

    mockSerial.write(0x55);
    mockSerial.write(0xCC);

    bool updated = radar.update();
    assert(updated == true);

    assert(radar.targets[0].active == false);
    assert(radar.targets[1].active == false);
    assert(radar.targets[2].active == false);

    std::cout << "  ✓ test_radar_parse_payload_empty passed" << std::endl;
}


void test_radar_get_delta_time_sec() {
    std::cout << "Running test_radar_get_delta_time_sec..." << std::endl;

    HardwareSerial mockSerial;
    E54_Radar radar(mockSerial);

    // Test default 10Hz fallback
    radar.frameDeltaMicros = 0;
    assert(std::abs(radar.getDeltaTimeSec() - 0.1f) < 1e-5);

    // Test valid microsecond conversion
    radar.frameDeltaMicros = 500000;
    assert(std::abs(radar.getDeltaTimeSec() - 0.5f) < 1e-5);

    radar.frameDeltaMicros = 1000000;
    assert(std::abs(radar.getDeltaTimeSec() - 1.0f) < 1e-5);

    std::cout << "  ✓ test_radar_get_delta_time_sec passed" << std::endl;
}

void test_radar_all() {
    std::cout << "Testing E54_Radar..." << std::endl;
    test_radar_parse_payload_positive_negative();
    test_radar_parse_payload_empty();
    test_radar_get_delta_time_sec();
    std::cout << "All E54_Radar tests passed!\n" << std::endl;
}
