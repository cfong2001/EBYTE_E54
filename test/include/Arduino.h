#ifndef MOCK_ARDUINO_H
#define MOCK_ARDUINO_H

#include <stdint.h>
#include <stdlib.h>
#include <algorithm>
#include <cmath>

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

#define SERIAL_8N1 0

class HardwareSerial {
public:
    void begin(unsigned long baud, uint32_t config, int8_t rxPin, int8_t txPin) {}
    int available() { return 0; }
    int read() { return -1; }
};

#endif
