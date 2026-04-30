#include <Arduino.h>
#include <TFT_eSPI.h>
#include <RotaryEncoder.h>
#include <OneButton.h>

#include "E54_Radar.h"
#include "MotionCompensation.h"
#include "UIManager.h"

// Hardware instances
HardwareSerial radarUART(1);
E54_Radar radar(radarUART);
MotionCompensation motionComp;

TFT_eSPI tft = TFT_eSPI();
UIManager ui(tft);

SemaphoreHandle_t dataMutex;

// Pins
#ifndef PIN_ENCODER_A
#define PIN_ENCODER_A 25
#endif
#ifndef PIN_ENCODER_B
#define PIN_ENCODER_B 26
#endif
#ifndef PIN_BUTTON
#define PIN_BUTTON    27
#endif
#ifndef RADAR_RX_PIN
#define RADAR_RX_PIN 16
#endif
#ifndef RADAR_TX_PIN
#define RADAR_TX_PIN 17
#endif

RotaryEncoder encoder(PIN_ENCODER_A, PIN_ENCODER_B, RotaryEncoder::LatchMode::TWO03);
OneButton button(PIN_BUTTON, true, true);

void IRAM_ATTR checkPosition() {
    encoder.tick();
}

void IRAM_ATTR checkButtonTicks() {
    button.tick();
}

void handleButtonPress() {
    ui.handleButton();
}

void radarTask(void *pvParameters) {
    while (1) {
#ifdef SIMULATION_MODE
        static float simAngle = 0;
        simAngle += 0.05f;
        RadarTarget simTargets[3];
        for(int i=0; i<3; i++) {
            simTargets[i].active = (i == 0);
            if (simTargets[i].active) {
                // Circular motion that spirals in/out to test zones
                float radius = 2500.0f + sinf(simAngle * 0.3f) * 1500.0f;
                simTargets[i].x = sinf(simAngle) * radius + (random(-15, 16));
                simTargets[i].y = radius + (random(-15, 16));
                simTargets[i].speed = 50 + (random(-2, 3));
            }
        }

        if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
            bool activeArr[3] = {simTargets[0].active, simTargets[1].active, simTargets[2].active};
            int16_t xArr[3] = {simTargets[0].x, simTargets[1].x, simTargets[2].x};
            int16_t yArr[3] = {simTargets[0].y, simTargets[1].y, simTargets[2].y};
            ui.zoneManager.updateFuzzing(activeArr, xArr, yArr);

            RadarTarget compensatedTargets[3];
            motionComp.process(simTargets, compensatedTargets);

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
        vTaskDelay(pdMS_TO_TICKS(100));
#else
        if (radar.update()) {
            if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
                bool activeArr[3] = {radar.targets[0].active, radar.targets[1].active, radar.targets[2].active};
                int16_t xArr[3] = {radar.targets[0].x, radar.targets[1].x, radar.targets[2].x};
                int16_t yArr[3] = {radar.targets[0].y, radar.targets[1].y, radar.targets[2].y};
                ui.zoneManager.updateFuzzing(activeArr, xArr, yArr);

                RadarTarget compensatedTargets[3];
                motionComp.process(radar.targets, compensatedTargets);

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
        vTaskDelay(pdMS_TO_TICKS(1));
#endif
    }
}

void setup() {
    dataMutex = xSemaphoreCreateMutex();
    Serial.begin(115200);
    Serial.println("System starting...");

    radar.begin(RADAR_RX_PIN, RADAR_TX_PIN);
    motionComp.init();
    ui.init();

    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_A), checkPosition, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_B), checkPosition, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_BUTTON), checkButtonTicks, CHANGE);
    button.attachClick(handleButtonPress);

    xTaskCreatePinnedToCore(radarTask, "RadarTask", 4096, NULL, 1, NULL, 0);
}

unsigned long lastRender = 0;

void loop() {
    button.tick();
    encoder.tick();
    int newPos = encoder.getPosition();
    static int lastPos = 0;
    if (newPos != lastPos) {
        ui.handleEncoder((int)(encoder.getDirection()));
        lastPos = newPos;
    }

    int act = ui.consumeAction();
    if (act == 1) {
        motionComp.forceReset();
        Serial.println(">> Tracking Reset.");
    }

    motionComp.setAveragingStrength(ui.getLocationAveraging());

    // Process radar (10Hz from E54)
    if (radar.update()) {
        bool activeArr[3] = {radar.targets[0].active, radar.targets[1].active, radar.targets[2].active};
        int16_t xArr[3] = {radar.targets[0].x, radar.targets[1].x, radar.targets[2].x};
        int16_t yArr[3] = {radar.targets[0].y, radar.targets[1].y, radar.targets[2].y};
        ui.zoneManager.updateFuzzing(activeArr, xArr, yArr);

        RadarTarget compensatedTargets[3];
        motionComp.process(radar.targets, compensatedTargets);

        for(int i=0; i<3; i++) {
            if (compensatedTargets[i].active && ui.zoneManager.isDead(compensatedTargets[i].x, compensatedTargets[i].y)) {
                compensatedTargets[i].active = false;
            }
        }
        ui.updateRadarData(compensatedTargets, motionComp.isAnchorValid(), motionComp.getAnchorX(), motionComp.getAnchorY());

        // Pass motion data (velocity/acceleration) to UI for curved predictive interpolation
        for (int i = 0; i < 3; i++) {
            ui.setTargetMotion(i, motionComp.getTargetVelX(i), motionComp.getTargetVelY(i),
                                  motionComp.getTargetAccX(i), motionComp.getTargetAccY(i));
        }

        if (ui.serialDebugEnabled) {
            Serial.print("RADAR_DEBUG|");
            Serial.print("AnchorValid:"); Serial.print(motionComp.isAnchorValid()); Serial.print(",");
            Serial.print("AnchorX:"); Serial.print(motionComp.getAnchorX()); Serial.print(",");
            Serial.print("AnchorY:"); Serial.print(motionComp.getAnchorY()); Serial.print("|");
            for (int i = 0; i < 3; i++) {
                Serial.print("T"); Serial.print(i); Serial.print(":");
                Serial.print(compensatedTargets[i].active); Serial.print(",");
                Serial.print(compensatedTargets[i].x); Serial.print(",");
                Serial.print(compensatedTargets[i].y); Serial.print(",");
                Serial.print(compensatedTargets[i].speed); Serial.print(i == 2 ? "" : "|");
            }
            Serial.println();
        }
    if (Serial.available()) {
        char cmd = Serial.read();
        if (cmd == 'l' || cmd == 'L') { ui.handleEncoder(-1); Serial.println(">> Sim: L"); }
        else if (cmd == 'r' || cmd == 'R') { ui.handleEncoder(1); Serial.println(">> Sim: R"); }
        else if (cmd == 'p' || cmd == 'P') { handleButtonPress(); Serial.println(">> Sim: P"); }
    }

    unsigned long now = millis();
    if (now - lastRender >= 30) {
        if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
            ui.renderLoop();
#ifdef SIMULATION_MODE
            static unsigned long lastLog = 0;
            if (now - lastLog >= 100) {
                Serial.printf("[%lu] State:%s Page:%s Danger:%.2f\n", now, ui.getStateName(), ui.getPageName(), ui.zoneManager.getDangerLevel());
                ui.logStateToSerial();
                lastLog = now;
            }
#endif
            xSemaphoreGive(dataMutex);
        }
        lastRender = now;
    }
}
