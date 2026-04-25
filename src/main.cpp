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

// Pins (Adjust as needed for the specific ESP32 board wiring)
#define PIN_ENCODER_A 25
#define PIN_ENCODER_B 26
#define PIN_BUTTON    27

RotaryEncoder encoder(PIN_ENCODER_A, PIN_ENCODER_B, RotaryEncoder::LatchMode::TWO03);
OneButton button(PIN_BUTTON, true, true);

// Interrupt routine for rotary encoder
void IRAM_ATTR checkPosition() {
    encoder.tick();
}

void handleButtonPress() {
    ui.handleButton();
}

void setup() {
    Serial.begin(115200);
    Serial.println("System starting...");

    // Initialize radar
    radar.begin(16, 17);
    motionComp.init();

    // Initialize UI
    ui.init();

    // Initialize inputs
    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_A), checkPosition, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_B), checkPosition, CHANGE);
    button.attachClick(handleButtonPress);
}

// Timer for render loop
unsigned long lastRender = 0;

void loop() {
    // Process button
    button.tick();

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
    }

    // Render loop (decoupled, max frame rate ~30-60Hz)
    unsigned long now = millis();
    if (now - lastRender >= 30) { // ~33Hz display rendering
        ui.renderLoop();
        lastRender = now;
    }
}
