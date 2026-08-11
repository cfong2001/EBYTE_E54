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

private:

    #if defined(ESP32)
    TaskHandle_t perfTaskHandle;
    #endif
    unsigned long lastSystemReportTime = 0;

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
