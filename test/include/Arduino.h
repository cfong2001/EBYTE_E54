#pragma once

#include <stdint.h>
#include <math.h>
#include <string>
#include <vector>
#include <cstdarg>
#include <cstdio>

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

class HardwareSerial {
public:
    void setRxBufferSize(int size) {}
    void begin(long baud, int config, int rx, int tx) {}
    int available() { return 0; }
    int read() { return -1; }
};

#define SERIAL_8N1 0

inline unsigned long micros() { return mock_millis * 1000; }
