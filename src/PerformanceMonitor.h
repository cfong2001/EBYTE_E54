#pragma once
#include <Arduino.h>
#if defined(ESP32)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif

class PerformanceMonitor {
public:
    void begin() {
        #if defined(ESP32)
        lastSystemReportTime = millis();
        lastMetricsReportTime = millis();
        xTaskCreatePinnedToCore(
            this->taskTrampoline,
            "PerfMonTask",
            4096,
            this,
            1, // Low priority
            &perfTaskHandle,
            tskNO_AFFINITY
        );
        #endif
    }

    void report() {
        unsigned long ms = millis();
        if (ms - lastMetricsReportTime >= 1000) {
            Serial.printf("[Perf] Frames: %d, UART errs: %d, Parse errs: %d\n",
                          framesThisSecond, uartErrorsThisSecond, parseErrorsThisSecond);
            framesThisSecond = 0;
            uartErrorsThisSecond = 0;
            parseErrorsThisSecond = 0;
            lastMetricsReportTime = ms;
        }
    }

    void incrementFrames() { framesThisSecond++; }
    void incrementUartErrors() { uartErrorsThisSecond++; }
    void incrementParseErrors() { parseErrorsThisSecond++; }

    // For testing purposes
    int getFrames() { return framesThisSecond; }

private:
    int framesThisSecond = 0;
    int uartErrorsThisSecond = 0;
    int parseErrorsThisSecond = 0;

    #if defined(ESP32)
    TaskHandle_t perfTaskHandle = nullptr;
    #endif
    unsigned long lastSystemReportTime = 0;
    unsigned long lastMetricsReportTime = 0;

    #if defined(ESP32)
    static void taskTrampoline(void *pvParameters) {
        PerformanceMonitor* instance = static_cast<PerformanceMonitor*>(pvParameters);
        instance->taskLoop();
    }

    void taskLoop() {
        while (1) {
            unsigned long now = millis();
            if (now - lastSystemReportTime > 5000) {
                // Detailed system status report
                Serial.println("\n--- FreeRTOS Performance & Remaining Power ---");
                Serial.printf("Task Count: %d\n", uxTaskGetNumberOfTasks());
                Serial.printf("Free Heap: %u bytes\n", ESP.getFreeHeap());
                Serial.printf("Max Allocatable Heap: %u bytes\n", ESP.getMaxAllocHeap());
                Serial.printf("Minimum Free Heap: %u bytes\n", ESP.getMinFreeHeap());
                Serial.println("---------------------------------------------------------\n");
                lastSystemReportTime = now;
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    #endif
};
