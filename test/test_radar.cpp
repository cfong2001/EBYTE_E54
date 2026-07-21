#include <iostream>
#include <cmath>
#include <cassert>
#include "E54_Radar.h"
#include "MotionCompensation.h"

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

void encodeValue(int16_t val, uint8_t* out) {
    uint16_t raw;
    if (val >= 0) {
        raw = (val & 0x7FFF) | 0x8000;
    } else {
        raw = (-val) & 0x7FFF;
    }
    out[0] = raw & 0xFF;
    out[1] = (raw >> 8) & 0xFF;
}

void injectFrame(HardwareSerial& serial, E54_Radar& radar, int16_t x1, int16_t y1, int16_t s1, int16_t x2, int16_t y2, int16_t s2, int16_t x3, int16_t y3, int16_t s3, bool active1 = true, bool active2 = true, bool active3 = true) {
    uint8_t payload[24] = {0};
    
    if (active1) {
        encodeValue(x1, &payload[0]);
        encodeValue(y1, &payload[2]);
        encodeValue(s1, &payload[4]);
        payload[6] = 5; // resolution
        payload[7] = 0;
    }
    if (active2) {
        encodeValue(x2, &payload[8]);
        encodeValue(y2, &payload[10]);
        encodeValue(s2, &payload[12]);
        payload[14] = 5;
        payload[15] = 0;
    }
    if (active3) {
        encodeValue(x3, &payload[16]);
        encodeValue(y3, &payload[18]);
        encodeValue(s3, &payload[20]);
        payload[22] = 5;
        payload[23] = 0;
    }

    serial.write(0xAA);
    serial.write(0xFF);
    serial.write(0x03);
    serial.write(0x00);
    serial.write(payload, 24);
    serial.write(0x55);
    serial.write(0xCC);

    radar.update();
}

void test_radar_mock_tracking_system() {
    std::cout << "Running test_radar_mock_tracking_system..." << std::endl;

    HardwareSerial mockSerial;
    E54_Radar radar(mockSerial);
    MotionCompensation mc;
    mc.init();

    RadarTarget compensated[3];

    // Frame 1: Initialize 3 targets with speed=50 to bypass anchor logic
    injectFrame(mockSerial, radar, 1000, 2000, 50, 2000, 3000, 50, 3000, 4000, 50);
    mc.process(0.1f, radar.targets, compensated);

    assert(compensated[0].active == true);
    assert(compensated[1].active == true);
    assert(compensated[2].active == true);
    assert(compensated[0].isCoasting == false);
    assert(compensated[1].isCoasting == false);
    assert(compensated[2].isCoasting == false);
    assert(compensated[0].x == 1000);
    assert(compensated[1].x == 2000);
    assert(compensated[2].x == 3000);

    // Frame 2: Move targets slightly
    injectFrame(mockSerial, radar, 1050, 2050, 50, 2050, 3050, 50, 3050, 4050, 50);
    mc.process(0.1f, radar.targets, compensated);

    assert(compensated[0].active == true);
    assert(compensated[1].active == true);
    assert(compensated[2].active == true);

    // Frame 3: Swap target 1 and target 2 slots in the raw payload
    injectFrame(mockSerial, radar, 2100, 3100, 50, 1100, 2100, 50, 3100, 4100, 50);
    mc.process(0.1f, radar.targets, compensated);

    // Assert positions are near target coordinates with 100mm tolerance (accounting for filtering/damping)
    assert(std::abs(compensated[0].x - 1100) < 100);
    assert(std::abs(compensated[1].x - 2100) < 100);
    assert(std::abs(compensated[2].x - 3100) < 100);
    assert(compensated[0].isCoasting == false);
    assert(compensated[1].isCoasting == false);
    assert(compensated[2].isCoasting == false);

    // Frame 4: Simulate a transient dropout for Target 3 (Track 2)
    injectFrame(mockSerial, radar, 1150, 2150, 50, 2150, 3150, 50, 0, 0, 0, true, true, false);
    mc.process(0.1f, radar.targets, compensated);

    assert(compensated[0].active == true);
    assert(compensated[1].active == true);
    assert(compensated[2].active == true); // Tracker should coast Target 3!
    assert(compensated[2].isCoasting == true);

    // Frame 5: Re-introducing Target 3 re-acquires it
    injectFrame(mockSerial, radar, 1200, 2200, 50, 2200, 3200, 50, 3200, 4200, 50);
    mc.process(0.1f, radar.targets, compensated);

    assert(compensated[2].active == true);
    assert(compensated[2].isCoasting == false); // Should be re-acquired
    assert(std::abs(compensated[2].x - 3200) < 100);

    // Dropouts lasting > 1.0s (15 frames) permanently deactivate the track
    for (int f = 0; f < 16; f++) {
        injectFrame(mockSerial, radar, 1200 + f*50, 2200 + f*50, 50, 2200 + f*50, 3200 + f*50, 50, 0, 0, 0, true, true, false);
        mc.process(0.1f, radar.targets, compensated);
    }

    assert(compensated[2].active == false); // Should be permanently dropped
    assert(compensated[2].isCoasting == false);

    std::cout << "  ✓ test_radar_mock_tracking_system passed" << std::endl;
}

void test_radar_all() {
    std::cout << "Testing E54_Radar..." << std::endl;
    test_radar_parse_payload_positive_negative();
    test_radar_parse_payload_empty();
    test_radar_get_delta_time_sec();
    test_radar_mock_tracking_system();
    std::cout << "All E54_Radar tests passed!\n" << std::endl;
}
