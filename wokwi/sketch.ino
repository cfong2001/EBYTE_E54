/*
 * Wokwi Simulation Sketch for ESP32 Radar Tracker
 *
 * This sketch simulates the HLK-LD2450 radar module by outputting a continuous
 * loop of fake radar data bytes over UART2.
 *
 * Instructions for Wokwi Web Editor:
 * 1. Create a new tab for each file in the `src/` folder (e.g. `main.cpp`, `E54_Radar.h`, etc.)
 * 2. Paste the contents of those files into the new Wokwi tabs.
 * 3. Inside your Wokwi `main.cpp`, you MUST call:
 *      setupSimulatedRadar(); inside `setup()`
 *      loopSimulatedRadar(); inside `loop()`
 *
 * If running locally using the Wokwi VS Code Extension, this file is ignored,
 * as Wokwi relies on `.pio/build/esp32-s3/firmware.elf` compiled natively.
 */

#include <Arduino.h>

// Radar UART Pins
#define RADAR_RX_PIN 32
#define RADAR_TX_PIN 4

HardwareSerial mockRadar(1);

void setupSimulatedRadar() {
  // Set up mock radar output on RADAR_RX_PIN (from ESP32's perspective, Radar TX is connected to ESP RX)
  // To simulate the radar, we will transmit on the pin the ESP expects to receive on.
  // So we transmit on RADAR_RX_PIN (32).
  mockRadar.begin(256000, SERIAL_8N1, 33, RADAR_RX_PIN);
}

void loopSimulatedRadar() {
  static uint32_t lastFrame = 0;
  if (millis() - lastFrame >= 100) {
    lastFrame = millis();
    // Transmit a simulated frame at ~10Hz
    uint8_t frame[30] = {
      0xAA, 0xFF, 0x03, 0x00, // Header 0-3

      // Target 1: x=500 (0x81F4), y=1000 (0x83E8), speed=0, res=0
      0xF4, 0x81, 0xE8, 0x83, 0x00, 0x00, 0x00, 0x00, // 4-11

      // Target 2: empty
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 12-19

      // Target 3: x=-200 (0x00C8), y=1500 (0x85DC), speed=0, res=0
      0xC8, 0x00, 0xDC, 0x85, 0x00, 0x00, 0x00, 0x00, // 20-27

      0x55, 0xCC // Tail 28-29
    };

    for (int i = 0; i < 30; i++) {
      mockRadar.write(frame[i]);
    }
  }
}

// In Wokwi Web, these overrides are required unless you modify your main.cpp to call them.
// If you include the src tabs, rename this tab to `SimulatedRadar.h` and `#include "SimulatedRadar.h"` in main.
#if !defined(MAIN_CPP_INCLUDED)
void setup() {
  Serial.begin(115200);
  Serial.println("Wokwi Simulation Environment Ready. Please include src/ files in tabs.");
  setupSimulatedRadar();
}

void loop() {
  loopSimulatedRadar();
}
#endif
