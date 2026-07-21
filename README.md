# EBYTE E54-24LD12D ESP32 Multi-Target Radar Tracker

An open-source, high-performance tactical radar tracking system built for the ESP32 microcontroller and the EBYTE E54-24LD12D 24GHz millimeter-wave (mmWave) radar sensor.

This project turns raw industrial radar signals into a portable, real-time spatial tracker that sees through obstacles, filters out hand motion, and broadcasts live target data directly to embedded displays and mobile web devices.

---

## Overview

Traditional optical cameras fail in complete darkness, generate privacy concerns, and cannot penetrate physical barriers. Ultrasonic sensors lack spatial accuracy and range, while thermal sensors struggle with ambient temperature fluctuations.

This firmware harnesses 24GHz FMCW radar technology to deliver high-resolution 2D spatial tracking of human targets. By analyzing millimeter-wave Doppler shifts, the system detects micro-movements, breathing, and walking motion through drywall, glass, enclosure plastics, and darkness.

Whether deployed as a portable handheld tracking unit, a perimeter alert monitor, or a privacy-preserving smart room sensor, the system provides accurate target distance, velocity, and angle telemetry without recording video or audio.

---

## Key Capabilities

- **Through-Barrier & Low-Light Motion Detection**: Detects human presence, micro-breathing, and directional movement through non-metallic walls, enclosures, and total darkness up to 30 meters.
- **Handheld & Vehicle Motion Compensation**: Advanced mathematical filtering isolates platform shake from true target motion, allowing the unit to be operated reliably while handheld or mounted on mobile rigs.
- **Smart Multi-Target Tracking**: Simultaneously tracks up to 3 distinct targets with Nearest-Neighbor spatial association, preventing target ID swapping or track dropping when subjects cross paths.
- **Dual Display Architecture**: Features a high-refresh-rate dynamic polar HUD on the attached color TFT screen while simultaneously hosting a secure local Wi-Fi web dashboard for remote monitoring on smartphones, tablets, or computers.
- **Privacy-Safe Spatial Monitoring**: Delivers exact 2D position coordinates (X/Y mm) and velocity (cm/s) without capturing identifiable personal imagery.
- **Ultra-Fast Display Engine**: Employs background sprite caching to eliminate redundant screen redraws, boosting display refresh rates up to 60-100 FPS for smooth target motion.

---

## Technical Specifications

| Parameter | Specification |
|---|---|
| Radar Frequency Band | 24.00 GHz to 24.25 GHz FMCW |
| Operational Range | 0.5 meters to 30.0 meters (2m dynamic auto-scaling) |
| Target Capacity | 3 simultaneous tracked targets |
| Sensor UART Baud Rate | 256,000 bps (Hardware auto-detected) |
| Display System | 240x320 SPI TFT Display (ST7789 / ILI9341) |
| Display Frame Rate | Up to 100 Hz (Background sprite cached) |
| Wireless Protocol | Wi-Fi 802.11 b/g/n SoftAP (`ESP32-Radar-Tracker`) |
| Web API Output Format | Real-time JSON stream (`/api/data`) |
| Input Controls | Quadrature Rotary Encoder (100ms debounced) & Tactile Buttons |
| Power Supply | 5V DC via USB-C or external regulator |

---

## System Architecture

The firmware utilizes a dual-core FreeRTOS architecture on the ESP32 to maintain high-throughput radar parsing alongside responsive UI rendering and web streaming.

```
+-------------------------------------------------------------------+
|                     ESP32 FreeRTOS Dual Core                      |
+-----------------------------------+-------------------------------+
| Core 0: High-Speed Radar Task     | Core 1: User Interface Task   |
| - 256,000 baud UART Stream Engine | - TFT_eSPI Background Caching |
| - Frame Sync (AA FF 03 00...55 CC)| - 60-100 FPS Polar Grid HUD   |
| - Nearest-Neighbor Target Matcher | - Async HTTP Web Server       |
| - Motion Compensation Filter      | - Rotary Encoder Handling     |
+-----------------------------------+-------------------------------+
                                    | Shared Thread-Safe Mutex Lock |
                                    +-------------------------------+
```

---

## Hardware Setup & Pin Connections

### Recommended Hardware
- ESP32-WROVER-IE or ESP32-DevKitC development board.
- EBYTE E54-24LD12D 24GHz mmWave Radar Module.
- 240x320 SPI TFT Color Display.
- EC11 Quadrature Rotary Encoder with integrated push button.

### Pinout Table

| Function | ESP32 GPIO | Connected Component | Description |
|---|---|---|---|
| Radar Serial RX | GPIO 32 | E54 Radar TX | High-speed telemetry input (Auto-detected) |
| Radar Serial TX | GPIO 4 | E54 Radar RX | Setup & mode configuration output |
| Rotary Encoder CLK | GPIO 25 | Encoder Channel A | Hardware pullup enabled |
| Rotary Encoder DT | GPIO 26 | Encoder Channel B | Hardware pullup enabled |
| Rotary Encoder Push | GPIO 27 | Encoder SW | Active-low click button |
| Secondary Button | GPIO 34 | Tactile KEY0 | Menu & navigation trigger |
| Display SPI CS | GPIO 15 | TFT CS | Chip Select |
| Display SPI DC | GPIO 2 | TFT DC / RS | Data / Command Control |
| Display SPI Reset | GPIO 4 | TFT RST | Panel Reset |

---

## Communication Protocol & Data Parsing

The EBYTE E54-24LD12D radar module outputs telemetry over serial UART using little-endian binary frames.

### Frame Layout

```
+--------------+------------------+------------------+------------------+------------+
| Header (4B)  | Target 1 (8B)    | Target 2 (8B)    | Target 3 (8B)    | Tail (2B)  |
+--------------+------------------+------------------+------------------+------------+
| AA FF 03 00  | X  Y  Spd  Res   | X  Y  Spd  Res   | X  Y  Spd  Res   | 55 CC      |
+--------------+------------------+------------------+------------------+------------+
```

### Telemetry Field Structure

Each target entry occupies 8 bytes:
- **X Position (int16)**: Lateral offset in millimeters. Bit 15 indicates direction (1 = Positive/Right, 0 = Negative/Left).
- **Y Position (int16)**: Straight-line distance in millimeters. Bit 15 indicates direction (1 = Positive/Forward, 0 = Negative/Rear).
- **Speed (int16)**: Radial velocity in centimeters per second (cm/s).
- **Resolution (uint16)**: Distance resolution metric in millimeters.

---

## Web Dashboard & Remote Streaming

When Wireless Broadcast Mode is enabled, the ESP32 hosts a self-contained web server over Wi-Fi (`ESP32-Radar-Tracker`). Connecting any smartphone, tablet, or laptop to the Wi-Fi network opens an interactive HUD.

### REST API Endpoints

#### GET `/`
Serves the responsive single-page HTML/SVG dashboard featuring polar grid arcs, dynamic range scaling, and real-time target position readouts.

#### GET `/api/data`
Returns live multi-target coordinate JSON telemetry. Includes `Access-Control-Allow-Origin: *` headers for cross-origin integration with custom dashboards or Home Assistant setups.

```json
{
  "targets": [
    {
      "active": true,
      "x": -188,
      "y": 1713,
      "speed": -16,
      "resolution": 330
    },
    {
      "active": false,
      "x": 0,
      "y": 0,
      "speed": 0,
      "resolution": 0
    },
    {
      "active": false,
      "x": 0,
      "y": 0,
      "speed": 0,
      "resolution": 0
    }
  ]
}
```

---

## Firmware Compilation & Flashing

This project is configured for building with PlatformIO.

### Compilation
To compile the firmware binary locally:

```bash
pio run -e esp32-wrover-ie
```

### Flashing Hardware
To upload the compiled firmware binary to an attached ESP32:

```bash
pio run -e esp32-wrover-ie -t upload
```

---

## Native Testing & Verification

The repository includes a host-native C++ unit test suite (`run_tests.exe`) to test signal processing filters, track association, payload parsing, and spatial boundary checking offline without hardware attached.

### Running Test Suite

```bash
g++ -I src test/test_radar.cpp -o test/run_tests.exe
.\test\run_tests.exe
```

---

## License

This project is licensed under the MIT License - see the LICENSE file for details.
