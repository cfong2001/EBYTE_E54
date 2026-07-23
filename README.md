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
- **Handheld & Vehicle Motion Compensation**: Advanced mathematical filtering isolates platform shake from true target motion using multi-target anchor centroids, allowing the unit to be operated reliably while handheld or mounted on mobile rigs.
- **Smart Multi-Target Tracking**: Simultaneously tracks up to 3 distinct targets with Nearest-Neighbor spatial association, preventing target ID swapping or track dropping when subjects cross paths.
- **1.0-Second Dropout Coasting Window**: Automatically interpolates target trajectories along predicted 2D velocity vectors for up to 1,000ms during transient signal dropouts, hiding real-track highlight rings while coasting.
- **Dual Display Architecture**: Features a high-refresh-rate dynamic polar HUD on the attached color TFT screen while simultaneously hosting a secure local Wi-Fi web dashboard for remote monitoring on smartphones, tablets, or computers.
- **5-Theme Modular Engine**: Provides customizable visual palettes (`Standard`, `Alien`, `Minimalist`, `Cyberpunk`, `Tactical Amber`) with smooth theme switching and NVS state persistence.
- **Privacy-Safe Spatial Monitoring**: Delivers exact 2D position coordinates (X/Y mm) and velocity (cm/s) without capturing identifiable personal imagery.
- **High-FPS Direct-to-Sprite Vector Engine**: Employs optimized vector rotation and polar grid rasterization directly on 16-bit TFT sprites, maintaining display refresh rates of 60 to 120 FPS.

---

## Technical Specifications

| Parameter | Specification |
|---|---|
| Radar Frequency Band | 24.00 GHz to 24.25 GHz FMCW |
| Operational Range | 0.5 meters to 30.0 meters (2m dynamic auto-scaling) |
| Target Capacity | 3 simultaneous tracked targets |
| Sensor UART Baud Rate | 256,000 bps (Hardware pin auto-detected) |
| Display System | 240x320 SPI TFT Display (ST7789 / ILI9341) |
| Display Frame Rate | 60 Hz to 120 Hz (Direct-to-Sprite Vector Engine) |
| Wireless Protocol | Wi-Fi 802.11 b/g/n SoftAP (`ESP32-Radar-Tracker`) |
| Web API Output Format | Real-time JSON stream (`/api/data`) |
| Input Controls | Quadrature Rotary Encoder (100ms debounced, `FOUR3` mode) & Tactile Buttons |
| Power Supply | 5V DC via USB-C or external regulator |

---

## System Architecture

The firmware utilizes a dual-core FreeRTOS architecture on the ESP32 to maintain high-throughput radar parsing alongside responsive UI rendering and web streaming.

```
+-------------------------------------------------------------------+
|                     ESP32 FreeRTOS Dual Core                      |
+-----------------------------------+-------------------------------+
| Core 0: High-Speed Radar Task     | Core 1: User Interface Task   |
| - 256,000 baud UART Stream Engine | - Direct-to-Sprite Renderer   |
| - Frame Sync (AA FF 03 00...55 CC)| - 60-120 FPS Polar Grid HUD   |
| - Nearest-Neighbor Target Matcher | - Async HTTP Web Server       |
| - Motion Compensation Filter      | - Rotary Encoder Handling     |
+-----------------------------------+-------------------------------+
                                    | Shared Thread-Safe Mutex Lock |
                                    +-------------------------------+
```

---

## Directory Structure

```
EBYTE_E54/
├── .github/
│   └── workflows/
│       └── c-cpp.yml              # Automated GitHub Actions CI workflow
├── boards/
│   └── e101-c6mn4-ps.json          # Custom board hardware manifest
├── src/                            # Production ESP32 C++ Firmware Source
│   ├── main.cpp                    # FreeRTOS dual-core entry point & setup
│   ├── E54_Radar.h                 # 256k baud E54 FMCW binary frame parser
│   ├── MotionCompensation.h        # Nearest-Neighbor matcher & 2D Motion Comp
│   ├── UIManager.h                 # 60-120 FPS TFT display HUD & vector grid
│   ├── ZoneManager.h               # Radial warning & dead zone evaluator
│   ├── Themes.h / Themes.cpp       # 5-theme dynamic color palette engine
│   ├── ConfigManager.h             # Deferred NVS preferences manager
│   ├── PerformanceMonitor.h        # Execution timing & metric tracker
│   └── BroadcastServer.h / .cpp    # Wi-Fi SoftAP & REST JSON stream server
├── test/                           # Native C++ Offline Unit Test Suite
│   ├── test_radar.cpp              # Test runner entry point
│   ├── test_main.cpp
│   ├── test_mocks.cpp
│   ├── test_motion_compensation.cpp
│   ├── test_performance_monitor.cpp
│   ├── test_zone_manager.cpp
│   ├── Makefile                    # Host compiler Makefile
│   └── include/                    # Mock Arduino & Preferences headers
├── wokwi/                          # Wokwi web simulator project files
├── .gitignore                      # Expanded build artifact ignore rules
├── LICENSE                         # MIT License
├── platformio.ini                  # PlatformIO environment configuration
└── README.md                       # Comprehensive project documentation
```

---

## Hardware Setup & Pin Connections

### Recommended Hardware
- ESP32-WROVER-IE or ESP32-DevKitC development board.
- EBYTE E54-24LD12D 24GHz mmWave Radar Module.
- 240x320 SPI TFT Color Display (ST7789 or ILI9341).
- EC11 Quadrature Rotary Encoder with integrated push button.

### Pinout Table

| Function | ESP32 GPIO | Connected Component | Description |
|---|---|---|---|
| Radar Serial RX | GPIO 32 | E54 Radar TX | High-speed telemetry input (Auto-detected with GPIO 4) |
| Radar Serial TX | GPIO 4 | E54 Radar RX | Setup & mode configuration output |
| Rotary Encoder CLK | GPIO 25 | Encoder Channel A | Hardware pullup enabled (`FOUR3` mode) |
| Rotary Encoder DT | GPIO 26 | Encoder Channel B | Hardware pullup enabled (`FOUR3` mode) |
| Rotary Encoder Push | GPIO 27 | Encoder SW | Active-low click button (100ms debounced) |
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

## Building & Flashing

This project is configured for building with PlatformIO (CLI or VS Code extension).

### Prerequisites
- [PlatformIO Core](https://docs.platformio.org/en/latest/core/index.html) or [VS Code PlatformIO Extension](https://platformio.org/platformio-for-vscode).
- GCC / G++ toolchain for host-native unit testing.

### Firmware Compilation
To compile the firmware binary locally:

```bash
pio run -e esp32-wrover-ie
```

### Flashing Hardware
To upload the compiled firmware binary to an attached ESP32:

```bash
pio run -e esp32-wrover-ie -t upload
```

### Serial Telemetry Monitor
To monitor serial logs and diagnostic output:

```bash
pio device monitor -b 115200
```

---

## Offline Unit Testing & Verification

The repository includes a host-native C++ unit test suite (`test/`) to test signal processing filters, track association, payload parsing, and spatial boundary checking offline without hardware attached.

### Compiling & Running Unit Tests

#### Windows (PowerShell / CMD)
```powershell
g++ -std=c++11 -I test/include -I src test/test_main.cpp test/test_mocks.cpp test/test_motion_compensation.cpp test/test_performance_monitor.cpp test/test_radar.cpp test/test_zone_manager.cpp -o test/run_tests.exe
.\test\run_tests.exe
```

#### Linux / macOS
```bash
make -C test test
```

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
