# ESP32 Radar Tracker with EBYTE E54-24LD12D

## Project Description

The ESP32 Radar Tracker is a real-time radar tracking system utilizing the EBYTE E54-24LD12D 24GHz mmWave radar sensor. It processes and visualizes target position data on an OLED or TFT display. The system features dual platform support, offering implementations in both C++ (PlatformIO framework) for optimal performance and CircuitPython for secondary prototyping and alternative visual interfaces. The core tracking system supports up to three simultaneous targets, interpolating their positions and compensating for motion.

## Hardware Requirements

- **Microcontroller**: ESP32-S3 (Tested on generic ESP32-S3 DevKitC-1 boards) or standard ESP32 (esp32dev).
- **Radar Sensor**: EBYTE E54-24LD12D 24GHz mmWave Radar Sensor.
- **Display**: 0.96" OLED (128x64, I2C, SSD1306) or compatible SPI TFT display.
- **Input**: Push-button rotary encoder, additional momentary push-button.

## Pinouts

The standard wiring configuration is as follows. Note that these may vary depending on the chosen board definition in the firmware configuration.

**Radar (EBYTE E54-24LD12D) to ESP32:**
- TX (Sensor) -> RX (ESP32 GPIO 16)
- RX (Sensor) -> TX (ESP32 GPIO 17)
- VCC -> 5V
- GND -> GND

**TFT Display (SPI - ESP32-S3 default):**
- MOSI -> GPIO 11
- SCLK -> GPIO 12
- CS -> GPIO 10
- DC -> GPIO 9
- RST -> GPIO 14

**Input Devices:**
- Encoder Pin A -> GPIO 25
- Encoder Pin B -> GPIO 26
- Main Button -> GPIO 27

## Folder Overview

The repository is structured to strictly separate hardware implementations, testing tools, and documentation.

- `arduino/`: Contains C++ projects intended for compilation via the PlatformIO framework.
- `circuitpython/`: Contains Python scripts for deployment directly to devices running the CircuitPython runtime.
- `utils/`: Contains PC-side testing tools, deployment scripts, and shared functionality definitions.
- `shared/`: Contains UI logic shared across different visualization modes.
- `src/`: Core C++ source files, including system initialization and main loop logic.
- `docs/`: Reference documentation, manuals, and protocol specifications.

## Build/Installation Guide

### C++ (PlatformIO) Build Instructions

The primary build environment utilizes PlatformIO.

1. Ensure Python 3 is installed on your system.
2. Install the PlatformIO core:
   `python3 -m pip install platformio`
3. Navigate to the root directory of the repository.
4. Compile the project for the target board using the specified environment:
   `pio run -e esp32-s3`
   (Substitute `esp32dev` if using the older generation hardware).
5. Upload the compiled firmware to the microcontroller:
   `pio run -e esp32-s3 -t upload`

### CircuitPython Deployment Instructions

If you intend to run the CircuitPython implementation, deploy scripts using the utility tool:

`python3 utils/deploy_circuitpython.py`

When using CircuitPython, hardware-specific scripts can be syntax checked locally using:
`python3 -m py_compile <filename.py>`

After executing tests or local py_compile checks, remove artifacts using `rm -rf __pycache__` to prevent bytecode contamination.

## Build Flags

The project configuration (`platformio.ini`) requires specific build flags to function correctly with the underlying libraries, specifically for the display driver and USB configurations.

### `esp32dev` Environment Build Flags
- `-DUSER_SETUP_LOADED=1`
- `-DST7789_DRIVER=1`
- `-DTFT_WIDTH=240`
- `-DTFT_HEIGHT=240`
- `-DTFT_MOSI=23`
- `-DTFT_SCLK=18`
- `-DTFT_CS=15`
- `-DTFT_DC=2`
- `-DTFT_RST=4`
- `-DLOAD_GLCD=1`
- `-DLOAD_FONT2=1`
- `-DLOAD_FONT4=1`
- `-DSPI_FREQUENCY=40000000`

### `esp32-s3` Environment Build Flags
- `-DARDUINO_USB_MODE=1`
- `-DARDUINO_USB_CDC_ON_BOOT=1`
- `-DUSER_SETUP_LOADED=1`
- `-DST7789_DRIVER=1`
- `-DTFT_WIDTH=240`
- `-DTFT_HEIGHT=240`
- `-DTFT_MOSI=11`
- `-DTFT_SCLK=12`
- `-DTFT_CS=10`
- `-DTFT_DC=9`
- `-DTFT_RST=14`
- `-DLOAD_GLCD=1`
- `-DLOAD_FONT2=1`
- `-DLOAD_FONT4=1`
- `-DSPI_FREQUENCY=40000000`

## Settings Wiki

Settings are managed via the rotary encoder and stored in NVS (`Preferences.h`).

### Menus
1. **Main Display View**: Default tracking interface.
2. **Options Menu**: Press encoder to access. Scroll to navigate.

### Parameters
- **Tracking Reset**: Recalibrates motion compensation filters.
- **Location Averaging**: Adjusts predictive smoothing strength (higher = less jitter, more latency).
