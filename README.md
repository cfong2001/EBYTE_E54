# ESP32 EBYTE E54 Multi-Target Radar Tracker

Firmware for the ESP32 microcontroller and the EBYTE E54-24LD12D 24 GHz millimeter-wave (mmWave) radar sensor.

This system processes 24 GHz FMCW radar telemetry to track the 2D position, velocity, and distance of up to three targets simultaneously. It renders a real-time polar grid display on an attached SPI TFT screen and streams JSON telemetry over a local Wi-Fi web interface.

---

## System Overview

The EBYTE E54-24LD12D sensor detects targets using 24 GHz millimeter-wave signals. Unlike optical cameras, mmWave radar operates in complete darkness and penetrates non-metallic materials such as drywall, plastic enclosures, and glass, without recording audio or video.

This firmware provides:
- **Binary Telemetry Parsing**: Decodes 256,000 baud little-endian UART frames from the E54 sensor.
- **Motion Compensation**: Filters out platform shake during handheld operation by calculating multi-target anchor centroids (active when two or more targets are present).
- **Target Coasting**: Extrapolates position coordinates for up to 1.0 second along predicted 2D velocity vectors during transient signal dropouts.
- **Dual Output**: Renders a vector-based polar HUD on a 240x320 TFT screen and serves a real-time web interface via an embedded SoftAP HTTP server.

---

## Technical Specifications

| Parameter | Specification |
|---|---|
| Radar Frequency Band | 24.00 GHz to 24.25 GHz FMCW |
| Sensor UART Baud Rate | 256,000 bps (Hardware pin auto-detected) |
| Maximum Targets | 3 simultaneous tracked targets |
| Detection Range | 0.5 m to 30.0 m (Auto-scaling 2m polar grid arcs) |
| Display Hardware | 240x320 SPI TFT Display (ST7789 / ILI9341) |
| Display Refresh Rate | 60 Hz to 120 Hz (Direct-to-sprite vector rendering) |
| Wireless Telemetry | Wi-Fi 802.11 b/g/n SoftAP (`ESP32-Radar-Tracker`) |
| Web API Output | JSON telemetry stream (`GET /api/data`) |
| User Interface Controls | EC11 Quadrature Encoder (100ms debounced, `FOUR3` mode) & Tactile Buttons |
| System Power | 5V DC via USB-C or external 5V rail |

---

## Firmware Architecture

The firmware assigns radar parsing and user interface rendering to separate FreeRTOS tasks across the ESP32's dual cores. Shared state is protected by a thread-safe mutex lock.

```
+-------------------------------------------------------------------+
|                     ESP32 FreeRTOS Dual Core                      |
+-----------------------------------+-------------------------------+
| Core 0: Radar Processing Task     | Core 1: User Interface Task   |
| - 256,000 baud UART parser        | - Direct-to-sprite TFT engine |
| - Frame sync validation           | - Polar grid HUD (60-120 FPS) |
| - Nearest-neighbor data matcher   | - Async HTTP web server       |
| - Motion compensation filter      | - Rotary encoder debouncer    |
+-----------------------------------+-------------------------------+
                                    | Thread-Safe State Mutex Lock  |
                                    +-------------------------------+
```

---

## Hardware Configuration & Pinout

### Components
- ESP32 development board (e.g., ESP32-WROVER-IE or ESP32-DevKitC).
- EBYTE E54-24LD12D 24 GHz mmWave radar module.
- 240x320 SPI TFT color display (ST7789 or ILI9341 controller).
- EC11 quadrature rotary encoder with integrated push button.

### Pinout Assignment

| Function | ESP32 GPIO | Connected Signal | Notes |
|---|---|---|---|
| Radar Serial RX | GPIO 32 | E54 Radar TX | Serial telemetry input (Auto-detected with GPIO 4) |
| Radar Serial TX | GPIO 4 | E54 Radar RX | Configuration output |
| Encoder Channel A | GPIO 25 | Encoder CLK | Hardware pull-up enabled |
| Encoder Channel B | GPIO 26 | Encoder DT | Hardware pull-up enabled |
| Encoder Switch | GPIO 27 | Encoder SW | Active-low, 100ms debounced |
| Secondary Button | GPIO 34 | Tactile KEY0 | Navigation input |
| TFT Chip Select | GPIO 15 | Display CS | SPI Chip Select |
| TFT Data/Command | GPIO 2 | Display DC | Data/Command selector |
| TFT Reset | GPIO 4 | Display RST | Hardware panel reset |

---

## Telemetry Protocol & Frame Structure

The E54 module outputs little-endian binary frames over UART.

### Binary Frame Layout

```
+--------------+------------------+------------------+------------------+------------+
| Header (4B)  | Target 1 (8B)    | Target 2 (8B)    | Target 3 (8B)    | Tail (2B)  |
+--------------+------------------+------------------+------------------+------------+
| AA FF 03 00  | X  Y  Spd  Res   | X  Y  Spd  Res   | X  Y  Spd  Res   | 55 CC      |
+--------------+------------------+------------------+------------------+------------+
```

### Target Data Fields

Each target block consists of 8 bytes:
- **X Position (`int16_t`)**: Lateral offset in millimeters (positive = right, negative = left).
- **Y Position (`int16_t`)**: Distance in millimeters (positive = forward, negative = rear).
- **Speed (`int16_t`)**: Radial velocity in centimeters per second (cm/s).
- **Resolution (`uint16_t`)**: Target resolution metric in millimeters.

---

## Web Telemetry Interface

When Wireless Broadcast Mode is enabled, the ESP32 creates a local Wi-Fi access point (`ESP32-Radar-Tracker`). Connected devices can view telemetry in a web browser.

### Endpoints

- **`GET /`**: Serves the single-page web dashboard with SVG polar grid visualization.
- **`GET /api/data`**: Returns real-time target coordinates as JSON. CORS headers (`Access-Control-Allow-Origin: *`) are enabled for integration with external dashboards.

#### Example JSON Response
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

## Building and Flashing

This project uses [PlatformIO](https://platformio.org/).

### Prerequisites
- PlatformIO Core or VS Code Extension.
- GCC/G++ toolchain (for native offline unit testing).

### Compile Firmware
```bash
pio run -e esp32-wrover-ie
```

### Flash Device
```bash
pio run -e esp32-wrover-ie -t upload
```

### Serial Monitor
```bash
pio device monitor -b 115200
```

---

## Host-Native Unit Testing

The repository contains a native C++ test suite under `test/` to verify signal processing, frame parsing, and zone math on host machines without an attached ESP32.

### Compile and Run Tests

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

## Directory Structure

```
EBYTE_E54/
├── .github/
│   └── workflows/
│       └── c-cpp.yml              # GitHub Actions CI build workflow
├── boards/
│   └── e101-c6mn4-ps.json          # Board definition file
├── src/                            # Firmware C++ source code
│   ├── main.cpp                    # Dual-core entry point and setup
│   ├── E54_Radar.h                 # E54 radar frame parser
│   ├── MotionCompensation.h        # Motion compensation & tracking algorithms
│   ├── UIManager.h                 # TFT display HUD and vector grid renderer
│   ├── ZoneManager.h               # Radial zone evaluator
│   ├── Themes.h / Themes.cpp       # 5-theme UI color engine
│   ├── ConfigManager.h             # Preferences manager
│   ├── PerformanceMonitor.h        # Metric tracking utility
│   └── BroadcastServer.h / .cpp    # Wi-Fi SoftAP and web server
├── test/                           # Host-native C++ unit test suite
│   ├── test_radar.cpp              # Test runner
│   ├── test_main.cpp
│   ├── test_mocks.cpp
│   ├── test_motion_compensation.cpp
│   ├── test_performance_monitor.cpp
│   ├── test_zone_manager.cpp
│   ├── Makefile                    # Test suite Makefile
│   └── include/                    # Mock headers for host compilation
├── wokwi/                          # Wokwi simulation configuration
├── .gitignore                      # Git ignore rules
├── LICENSE                         # License file (CC BY-NC-SA 4.0)
├── platformio.ini                  # PlatformIO configuration
└── README.md                       # Project documentation
```

---

## License

This project is licensed under the **Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0)**.

- **Non-Commercial**: Free for open-source community, educational, and personal non-commercial use. Commercial distribution or integration requires permission.
- **Share-Alike**: Derivative works and modified versions must be released under the same CC BY-NC-SA 4.0 license terms.

See [LICENSE](LICENSE) for legal details.
