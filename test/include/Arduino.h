#pragma once

#include <stdint.h>
#include <math.h>
#include <string>
#include <vector>
#include <cstdarg>
#include <cstdio>
#include <algorithm>

using std::min;
using std::max;

#define PI 3.14159265358979323846

// Mock Serial
class MockSerial {
public:
    std::vector<std::string> log;

    void println(const char* s) {
        log.push_back(std::string(s));
    }

    void printf(const char* format, ...) {
        char buf[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buf, sizeof(buf), format, args);
        va_end(args);
        log.push_back(std::string(buf));
    }

    void clear() {
        log.clear();
    }

    operator bool() const {
        return true;
    }
};

extern MockSerial Serial;

// Mock millis
extern unsigned long mock_millis;
inline unsigned long millis() { return mock_millis; }

// Mock ESP
class MockESP {
public:
    uint32_t getFreeHeap() { return 100000; }
    uint32_t getMaxAllocHeap() { return 50000; }
    uint32_t getMinFreeHeap() { return 20000; }
};
extern MockESP ESP;

// FreeRTOS Mocks/Stubs
typedef void* TaskHandle_t;
#define tskNO_AFFINITY 0
#define pdMS_TO_TICKS(ms) (ms)

inline int uxTaskGetNumberOfTasks() { return 1; }
inline void vTaskDelay(int ticks) {}
inline void xTaskCreatePinnedToCore(void (*task)(void*), const char* name, int stack, void* param, int prio, TaskHandle_t* handle, int core) {}


#ifndef ARDUINO_H_ADDITIONS
#define ARDUINO_H_ADDITIONS
#ifndef SERIAL_8N1
#define SERIAL_8N1 0x800001c
#endif

class HardwareSerial {
public:
    void setRxBufferSize(int size) {}
    void begin(long baud, int config, int rx, int tx) {}
    int available() { return 0; }
    int read() { return -1; }
    void begin(unsigned long baud, uint32_t config=SERIAL_8N1, int8_t rxPin=-1, int8_t txPin=-1, bool invert=false, unsigned long timeout_ms = 20000UL) {}
    void setRxBufferSize(size_t) {}
    int available() { return 0; }
    int read() { return -1; }
    size_t write(uint8_t) { return 1; }
    size_t write(const uint8_t *buffer, size_t size) { return size; }
};

#ifndef micros
inline unsigned long micros() { return mock_millis * 1000; }
#endif

#endif

namespace std {
    template<class T>
    constexpr const T& clamp(const T& v, const T& lo, const T& hi) {
        return (v < lo) ? lo : (hi < v) ? hi : v;
    }
}
