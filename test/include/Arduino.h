#pragma once

#include <stdint.h>
#include <math.h>
#include <string>
#include <vector>
#include <cstdarg>
#include <cstdio>
#include <queue>

#define PI 3.14159265358979323846
#define SERIAL_8N1 0x800001c

// Mock micros
extern uint32_t mock_micros;
inline uint32_t micros() { return mock_micros; }

class HardwareSerial {
public:
    std::queue<uint8_t> rx_buffer;

    void setRxBufferSize(size_t size) {}
    void begin(long baudRate, uint32_t config, uint8_t rxPin, uint8_t txPin) {}

    int available() {
        return rx_buffer.size();
    }

    int read() {
        if (rx_buffer.empty()) return -1;
        uint8_t val = rx_buffer.front();
        rx_buffer.pop();
        return val;
    }

    void write(uint8_t b) {
        rx_buffer.push(b);
    }
};

// Mock Serial
class MockSerial {
public:
    std::vector<std::string> log;

    void print(const char* s) {}
    template<typename T> void println(T t) {}
    template<typename T> void print(T t) {}
    operator bool() const { return true; }
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

    explicit operator bool() const { return true; }

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

// Update MockSerial to support operator bool()
class HardwareSerial {
public:
    void setRxBufferSize(size_t size) {}
    void begin(long baud, int config, int rxPin, int txPin) {}
    int available() { return 0; }
    int read() { return 0; }
};

#define SERIAL_8N1 0

inline unsigned long micros() { return mock_millis * 1000; }

typedef void* SemaphoreHandle_t;
inline SemaphoreHandle_t xSemaphoreCreateMutex() { return (SemaphoreHandle_t)1; }
inline bool xSemaphoreTake(SemaphoreHandle_t xSemaphore, int xTicksToWait) { return true; }
inline bool xSemaphoreGive(SemaphoreHandle_t xSemaphore) { return true; }
