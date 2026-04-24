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
        RadarTarget compensatedTargets[3];
        motionComp.process(radar.targets, compensatedTargets);
        ui.updateRadarData(compensatedTargets, motionComp.isAnchorValid(), motionComp.getAnchorX(), motionComp.getAnchorY());
    }

    // Render loop (decoupled, max frame rate ~30-60Hz)
    unsigned long now = millis();
    if (now - lastRender >= 30) { // ~33Hz display rendering
        ui.renderLoop();
        lastRender = now;
    }
}
