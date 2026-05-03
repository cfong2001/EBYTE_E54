#ifndef E54_RADAR_H
#define E54_RADAR_H

#include <Arduino.h>

struct RadarTarget {
    bool active;
    int16_t x;          // mm
    int16_t y;          // mm
    int16_t speed;      // cm/s
    uint16_t resolution; // mm
};

class E54_Radar {
public:
    bool passthroughMode = false;
    E54_Radar(HardwareSerial& serial) : radarSerial(serial) {}

    void begin(uint8_t rxPin, uint8_t txPin, long baudRate = 256000) {
        radarSerial.begin(baudRate, SERIAL_8N1, rxPin, txPin);
    }

    bool update() {
        bool updated = false;
        while (radarSerial.available()) {
            uint8_t b = radarSerial.read();
            if (passthroughMode) {
                Serial.write(b);
            }
            if (processByte(b)) {
                updated = true;
            }
        }
        return updated;
    }

    RadarTarget targets[3];

private:
    HardwareSerial& radarSerial;
    uint8_t buffer[64];
    uint8_t bufIndex = 0;

    enum State {
        SYNC_1, SYNC_2, SYNC_3, SYNC_4,
        PAYLOAD,
        TAIL_1, TAIL_2
    } state = SYNC_1;

    bool processByte(uint8_t b) {
        switch (state) {
            case SYNC_1:
                if (b == 0xAA) state = SYNC_2;
                break;
            case SYNC_2:
                if (b == 0xFF) state = SYNC_3;
                else state = SYNC_1;
                break;
            case SYNC_3:
                if (b == 0x03) state = SYNC_4;
                else state = SYNC_1;
                break;
            case SYNC_4:
                if (b == 0x00) {
                    state = PAYLOAD;
                    bufIndex = 0;
                }
                else state = SYNC_1;
                break;
            case PAYLOAD:
                if (bufIndex < sizeof(buffer)) {
                    buffer[bufIndex++] = b;
                }
                // Target 1: 8 bytes, Target 2: 8 bytes, Target 3: 8 bytes = 24 bytes
                if (bufIndex == 24) {
                    state = TAIL_1;
                }
                break;
            case TAIL_1:
                if (b == 0x55) state = TAIL_2;
                else state = SYNC_1;
                break;
            case TAIL_2:
                if (b == 0xCC) {
                    parsePayload();
                    state = SYNC_1;
                    return true;
                } else {
                    state = SYNC_1;
                }
                break;
        }
        return false;
    }

    void parsePayload() {
        for (int i = 0; i < 3; i++) {
            int offset = i * 8;
            uint16_t xRaw = buffer[offset] | (buffer[offset + 1] << 8);
            uint16_t yRaw = buffer[offset + 2] | (buffer[offset + 3] << 8);
            uint16_t sRaw = buffer[offset + 4] | (buffer[offset + 5] << 8);
            uint16_t rRaw = buffer[offset + 6] | (buffer[offset + 7] << 8);

            if (xRaw == 0 && yRaw == 0 && sRaw == 0 && rRaw == 0) {
                targets[i].active = false;
                continue;
            }

            targets[i].active = true;
            targets[i].x = decodeValue(xRaw);
            targets[i].y = decodeValue(yRaw);
            targets[i].speed = decodeValue(sRaw);
            targets[i].resolution = rRaw;
        }
    }

    int16_t decodeValue(uint16_t raw) {
        // High bit indicates positive (1) or negative (0)
        // Note: The example in the manual says:
        // Target 1x coordinate: 0x0E + 0x03 * 256 = 782 -> 0 - 782 = -782mm
        // Target 1 y coordinate: 0xB1 + 0x86 * 256 = 34481 -> 34481 - 2^15 = 1713 mm

        if (raw == 0) return 0;

        bool isPositive = (raw & 0x8000) != 0;
        int16_t value = raw & 0x7FFF;
        if (!isPositive) {
            value = -value;
        }
        return value;
    }
};

#endif
