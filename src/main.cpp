#include <Arduino.h>
#include <TFT_eSPI.h>
#include <RotaryEncoder.h>
#include <OneButton.h>

#include "E54_Radar.h"
#include "MotionCompensation.h"
#include "UIManager.h"
#include "PerformanceMonitor.h"

// Hardware instances
HardwareSerial radarUART(1);
E54_Radar radar(radarUART);
MotionCompensation motionComp;
PerformanceMonitor perfMonitor;

TFT_eSPI tft = TFT_eSPI();
UIManager ui(tft);

SemaphoreHandle_t dataMutex;

// Pins (Adjust as needed for the specific ESP32 board wiring)
#ifndef PIN_ENCODER_A
#define PIN_ENCODER_A 25
#endif

#ifndef PIN_ENCODER_B
#define PIN_ENCODER_B 26
#endif

#ifndef PIN_BUTTON
#define PIN_BUTTON    27
#endif

#ifndef PIN_KEY0
#define PIN_KEY0      0
#endif

#ifndef RADAR_RX_PIN
#define RADAR_RX_PIN 16
#endif
#ifndef RADAR_TX_PIN
#define RADAR_TX_PIN 17
#endif

RotaryEncoder encoder(PIN_ENCODER_A, PIN_ENCODER_B, RotaryEncoder::LatchMode::TWO03);
OneButton button(PIN_BUTTON, true, true);
OneButton key0(PIN_KEY0, true, true);

// Interrupt routine for rotary encoder
void IRAM_ATTR checkPosition() {
    encoder.tick();
}

void handleButtonPress() {
    ui.handleButton();
}

void handleButtonLongPressStart() {
    ui.handleButtonLongPress();
}

void handleKey0Press() {
    Serial.println("KEY0 pressed.");
}

void radarTask(void *pvParameters) {
    while (1) {
        if (radar.update()) {
            if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
                bool activeArr[3] = {radar.targets[0].active, radar.targets[1].active, radar.targets[2].active};
                int16_t xArr[3] = {radar.targets[0].x, radar.targets[1].x, radar.targets[2].x};
                int16_t yArr[3] = {radar.targets[0].y, radar.targets[1].y, radar.targets[2].y};
                ui.zoneManager.updateFuzzing(activeArr, xArr, yArr);

                RadarTarget compensatedTargets[3];
                if (ui.motionCompEnabled) {
                    motionComp.process(radar.targets, compensatedTargets);
                } else {
                    for(int i=0; i<3; i++) {
                        compensatedTargets[i] = radar.targets[i];
                    }
                }

                for(int i=0; i<3; i++) {
                    if (compensatedTargets[i].active && ui.zoneManager.isDead(compensatedTargets[i].x, compensatedTargets[i].y)) {
                        compensatedTargets[i].active = false;
                    }
                }
                ui.updateRadarData(compensatedTargets, motionComp.isAnchorValid(), motionComp.getAnchorX(), motionComp.getAnchorY());

                for (int i = 0; i < 3; i++) {
                    ui.setTargetMotion(i, motionComp.getTargetVelX(i), motionComp.getTargetVelY(i),
                                          motionComp.getTargetAccX(i), motionComp.getTargetAccY(i));
                }
                xSemaphoreGive(dataMutex);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1)); // Yield to other tasks
    }
}

void setup() {
    dataMutex = xSemaphoreCreateMutex();
    Serial.begin(115200);
    Serial.println("System starting...");

#ifdef TFT_BLK
    pinMode(TFT_BLK, OUTPUT);
    digitalWrite(TFT_BLK, HIGH);
#endif

    // Initialize radar
    radar.begin(RADAR_RX_PIN, RADAR_TX_PIN);
    motionComp.init();
    perfMonitor.begin();

    // Initialize UI
    ui.init();

    // Initialize inputs
    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_A), checkPosition, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_B), checkPosition, CHANGE);
button.attachClick(handleButtonPress);
    button.attachLongPressStart(handleButtonLongPressStart);
    key0.attachClick(handleKey0Press);

    xTaskCreatePinnedToCore(
        radarTask,   /* Task function. */
        "RadarTask", /* String with name of task. */
        4096,        /* Stack size in bytes. */
        NULL,        /* Parameter passed as input of the task */
        1,           /* Priority of the task. */
        NULL,        /* Task handle. */
        0);          /* Core where the task should run */
}

// Timer for render loop
unsigned long lastRender = 0;

void loop() {
    // Process button
    button.tick();
    key0.tick();

    // Process encoder
    encoder.tick();
    int newPos = encoder.getPosition();
    static int lastPos = 0;
    if (newPos != lastPos) {
        int dir = (int)(encoder.getDirection());
        ui.handleEncoder(dir);
        lastPos = newPos;
    }

    // Process UI Actions
    int act = ui.consumeAction();
    if (act == 1) { // Reset Tracking
        motionComp.forceReset();
        Serial.println("Motion Compensation Tracking Reset.");
    }

// Apply Settings
    motionComp.setAveragingStrength(ui.getLocationAveraging());

    radar.passthroughMode = ui.passthroughMode;

    if (ui.passthroughMode && Serial) {
        while (Serial.available()) {
            radarUART.write(Serial.read());
        }
    }

    // Render loop (decoupled, max frame rate ~30-60Hz)
    unsigned long now = millis();
    if (now - lastRender >= 30) { // ~33Hz display rendering
        if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
            ui.renderLoop();
            xSemaphoreGive(dataMutex);
        }
        lastRender = now;
    }
}
