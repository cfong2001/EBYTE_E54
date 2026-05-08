#include <Arduino.h>
#include <TFT_eSPI.h>
#include <RotaryEncoder.h>
#include <OneButton.h>

#include "E54_Radar.h"
#include "MotionCompensation.h"
#include "UIManager.h"
#include "PerformanceMonitor.h"
#include "ConfigManager.h"

ConfigManager configManager;

// Hardware instances
HardwareSerial radarUART(2);
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
#define PIN_BUTTON    27  // PUSH - encoder integrated button (module has 10k pullup)
#endif

#ifndef PIN_KEY0
#define PIN_KEY0      34  // KEY0 - secondary menu/confirm button (module has 10k pullup)
                          // GPIO34 is input-only on ESP32 -- external pullup required (module provides it)
#endif

#ifndef RADAR_RX_PIN
#define RADAR_RX_PIN 32  // Radar TX -> ESP32 RX
#endif
#ifndef RADAR_TX_PIN
#define RADAR_TX_PIN 4   // Radar RX -> ESP32 TX
#endif

RotaryEncoder encoder(PIN_ENCODER_A, PIN_ENCODER_B, RotaryEncoder::LatchMode::TWO03);
// Module has 10k hardware pullups on PUSH, A, B, KEY0 -- do NOT enable ESP32 internal pullups (pullupActive=false)
OneButton button(PIN_BUTTON, true, false);  // PUSH: active-low, external pullup on module
OneButton key0(PIN_KEY0,    true, false);   // KEY0: active-low, external pullup on module

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

// KEY0: secondary "menu return / confirmation" button per module datasheet
void handleKey0Press() {
    // In menu: acts as back/confirm (same as encoder press for simplicity)
    // In radar view: same as encoder press (open menu)
    ui.handleButton();
}

void handleKey0LongPress() {
    // Long-press KEY0: open the guide screen
    if (ui.state == STATE_RADAR_VIEW) {
        ui.state = STATE_GUIDE;
        ui.guidePage = 0;
    }
}

void radarTask(void *pvParameters) {
    static uint32_t lastHeartbeat = 0;
    static uint32_t totalFrames = 0;

    while (1) {
        // NOTE: hex dump is captured inside radar.update() via rawLogBuf[]
        // so the parser always sees every byte first.
        if (radar.update()) {
            totalFrames++;

            int activeCount = 0;
            for (int i = 0; i < 3; i++) {
                if (radar.targets[i].active) activeCount++;
            }

            // --- RADAR REPORTING ---
            if (activeCount > 0) {
                Serial.printf("[RADAR] Frame #%lu | %d Active |", totalFrames, activeCount);
                for (int i = 0; i < 3; i++) {
                    if (radar.targets[i].active) {
                        Serial.printf(" T%d(X:%dmm Y:%dmm Spd:%dcm/s)", i+1,
                            radar.targets[i].x,
                            radar.targets[i].y,
                            radar.targets[i].speed);
                    }
                }
                Serial.println();
            } else if (totalFrames % 100 == 0) {
                Serial.printf("[RADAR] Frame #%lu | No targets detected\n", totalFrames);
            }

            if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
                bool activeArr[3] = {radar.targets[0].active, radar.targets[1].active, radar.targets[2].active};
                int16_t xArr[3] = {radar.targets[0].x, radar.targets[1].x, radar.targets[2].x};
                int16_t yArr[3] = {radar.targets[0].y, radar.targets[1].y, radar.targets[2].y};
                ui.zoneManager.updateFuzzing(activeArr, xArr, yArr);

                RadarTarget compensatedTargets[3];
                if (ui.motionCompEnabled) {
                    float dt = radar.getDeltaTimeSec();
                    motionComp.process(dt, radar.targets, compensatedTargets);
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
                                          motionComp.getTargetAccX(i), motionComp.getTargetAccY(i), motionComp.getTargetStdDev(i));
                }
                xSemaphoreGive(dataMutex);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

unsigned long fallbackStart = 0;
String serialBuffer = "";

void setup() {
    dataMutex = xSemaphoreCreateMutex();
    Serial.begin(115200);
    Serial.println("ESP32 Radar Tracker Starting...");



    // Fallback logic
    if (configManager.checkFallback()) {
        ui.state = STATE_FALLBACK;
        fallbackStart = millis();
    }

    // Initialize radar
    Serial.println("--- RADAR WIRING CHECK ---");
    Serial.printf("Connect Sensor TX -> ESP32 GPIO %d\n", RADAR_RX_PIN);
    Serial.printf("Connect Sensor RX -> ESP32 GPIO %d\n", RADAR_TX_PIN);
    Serial.printf("Baud Rate: %d\n", RADAR_BAUD);
    Serial.println("--------------------------");
    
    // TEST: Disable internal pull-up on RX to see if Radar drives it HIGH
    pinMode(RADAR_RX_PIN, INPUT); 
    pinMode(RADAR_TX_PIN, OUTPUT);
    digitalWrite(RADAR_TX_PIN, HIGH);

    Serial.println("--- ELECTRICAL CHECK ---");
    delay(100);
    if (digitalRead(RADAR_RX_PIN) == LOW) {
        Serial.println("!! WARNING: RX pin is LOW. Check Power/GND.");
    } else {
        Serial.println("OK: RX pin is HIGH (Idle). Signal path is active.");
    }

    // Double-Baud Handshake
    uint8_t startCmd[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x02, 0x00, 0x90, 0x00, 0x04, 0x03, 0x02, 0x01};
    
    Serial.println("Handshake Step 1: 115200 baud...");
    // Optimization: Increase RX buffer size from default 256 to 1024 to prevent overflow and packet loss at high baud rates (115200+)
    radarUART.setRxBufferSize(1024);
    radarUART.begin(115200, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);
    radarUART.write(startCmd, sizeof(startCmd));
    delay(200);

    Serial.println("Handshake Step 2: 256000 baud...");
    radarUART.begin(256000, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);
    radarUART.write(startCmd, sizeof(startCmd));
    delay(200);

    radar.begin(RADAR_RX_PIN, RADAR_TX_PIN, 256000);
    
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
    key0.attachLongPressStart(handleKey0LongPress);

    xTaskCreatePinnedToCore(
        radarTask,   /* Task function. */
        "RadarTask", /* String with name of task. */
        4096,        /* Stack size in bytes. */
        NULL,        /* Parameter passed as input of the task */
        1,           /* Priority of the task. */
        NULL,        /* Task handle. */
        0);          /* Core where the task should run */

    Serial.println("Setup complete. Monitoring...");
}

// Timer for render loop
unsigned long lastRender = 0;

void loop() {
    // Process buttons
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
    } else if (act == 2) {
        configManager.exportConfig(ui);
    } else if (act == 3) {
        // Confirmed fallback
        Serial.println("New config confirmed.");
    }

    if (ui.state == STATE_IMPORTING) {
        while (Serial.available()) {
            char c = Serial.read();
            serialBuffer += c;
            if (serialBuffer.endsWith("}")) {
                configManager.importConfig(serialBuffer);
                serialBuffer = "";
            }
        }
    } else if (ui.state == STATE_FALLBACK) {
        if (millis() - fallbackStart > 15000) { // 15s timeout
            Serial.println("Fallback timeout. Reverting...");
            configManager.restoreFromFallback();
            delay(1000);
            ESP.restart();
        }
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
            ui.zoneManager.handleDeferredSave();
            ui.renderLoop();
            xSemaphoreGive(dataMutex);
        }
        lastRender = now;
    }
}
