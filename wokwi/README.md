# Wokwi Simulation Guide for ESP32 Radar Tracker

This folder provides the layout and simulation mockups to run the project in Wokwi, since a physical HLK-LD2450 mmWave radar cannot be simulated in the browser.

## Option 1: Wokwi Web Editor
1. Go to [Wokwi](https://wokwi.com) and start a new ESP32 project.
2. Copy the contents of `wokwi/diagram.json` into the `diagram.json` tab.
3. Copy the contents of `wokwi/libraries.txt` into the `libraries.txt` tab.
4. Copy the contents of `wokwi/sketch.ino` into your `sketch.ino` tab.
5. **Include Core Logic**: In Wokwi, create new tabs for the core application files found in `../src/` (e.g., `main.cpp`, `E54_Radar.h`, `UIManager.h`, `MotionCompensation.h`, `ConfigManager.h`, `PerformanceMonitor.h`). Paste their contents into the respective new tabs.
6. **Rename**: Rename your main Wokwi tab to `sketch.ino` if it isn't already. `sketch.ino` provides the simulated `setupSimulatedRadar()` and `loopSimulatedRadar()` which injects fake target bytes into the UART buffer so your core logic has data to process.

## Option 2: Wokwi VS Code Extension (Local)
If you have the Wokwi extension installed in VS Code, you can simulate this project directly from your local files!
1. Wokwi integrates natively with PlatformIO.
2. The `wokwi.toml` file instructs Wokwi to use the compiled `firmware.elf` from `.pio/build/esp32-s3/`.
3. Open the Command Palette in VS Code (`Ctrl+Shift+P`) and select `Wokwi: Start Simulator`. It will use `wokwi/diagram.json` to draw the UI and hook into your local code!
