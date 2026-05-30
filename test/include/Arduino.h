#pragma once

#include <stdint.h>
#include <math.h>
#include <string>
#include <vector>
#include <cstdarg>
#include <cstdio>
#include <algorithm>
#include <queue>

class String : public std::string {
public:
    String() : std::string() {}
    String(const char* s) : std::string(s) {}
    String(const std::string& s) : std::string(s) {}

    String& operator+=(const char* s) {
        std::string::operator+=(s);
        return *this;
    }

    String& operator+=(char c) {
        std::string::operator+=(c);
        return *this;
    }

    size_t length() const { return std::string::length(); }
    const char* c_str() const { return std::string::c_str(); }

    bool startsWith(const char* prefix) const {
        return std::string::find(prefix) == 0;
    }
    bool startsWith(const String& prefix) const {
        return std::string::find(prefix.c_str()) == 0;
    }

};


using std::min;
using std::max;

#define PI 3.14159265358979323846

#include <cstdlib>
inline long random(long min, long max) {
    if (min >= max) return min;
    long diff = max - min;
    return min + (std::rand() % diff);
}
inline long random(long max) {
    if (max == 0) return 0;
    return std::rand() % max;
}


// Mock Serial
class MockSerial {
public:
    std::vector<std::string> log;

    void print(const char* s) {
        log.push_back(std::string(s));
    }
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
    void restart() {}
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
    std::queue<uint8_t> rx_buffer;

    void begin(unsigned long baud, uint32_t config=SERIAL_8N1, int8_t rxPin=-1, int8_t txPin=-1, bool invert=false, unsigned long timeout_ms = 20000UL) {}
    void setRxBufferSize(size_t) {}

    int available() { return rx_buffer.size(); }
    int read() {
        if (rx_buffer.empty()) return -1;
        uint8_t val = rx_buffer.front();
        rx_buffer.pop();
        return val;
    }
    size_t write(uint8_t b) {
        rx_buffer.push(b);
        return 1;
    }
    size_t write(const uint8_t *buffer, size_t size) {
        for(size_t i=0; i<size; i++) rx_buffer.push(buffer[i]);
        return size;
    }
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

inline void delay(unsigned long) {}
