# EBYTE E54-24LD12D ESP32 Multi-Target Radar Tracker

An open-source, high-performance firmware implementation for real-time 24GHz mmWave radar target tracking using the ESP32 microcontroller and the EBYTE E54-24LD12D radar module.

This project provides real-time 2D spatial tracking, multi-target data association, stationary anchor motion compensation, dynamic polar coordinate rendering, web-based broadcast streaming, and non-volatile configuration management.

---

## Features

- **Hardware Multi-Target Radar Interface**: Interfaces with the EBYTE E54-24LD12D 24GHz FMCW radar module over UART at 256,000 bps using little-endian binary frame decoding (`AA FF 03 00 ... 55 CC`).
- **Nearest-Neighbor Spatial Data Association**: Tracks up to 3 simultaneous targets without ID swapping or aggressive track rejection using distance matrix greedy matching.
- **Stationary Anchor Motion Compensation**: Filters platform shake using relative geometry when 2 or more stationary anchors are detected, bypassing single-target coordinate lock.
- **Dynamic Polar Coordinate Display**: Renders concentric distance arcs (even 2m heavy strokes, odd 1m light strokes) with automatic dynamic range scaling bounded between 10m and 30m.
- **Rotary Encoder & Button UI**: Hardware-debounced rotary encoder and tactile button navigation with 5 selectable theme palettes (Standard, Alien, Minimalist, Cyberpunk, Tactical Amber).
- **Wireless Broadcast Web Server**: Embedded Async HTTP server streaming real-time JSON target coordinates (`/api/data`) and hosting an interactive SVG polar radar dashboard over Wi-Fi SoftAP (`ESP32-Radar-Tracker`).
- **Hardware Auto-Detection**: Boot sequence auto-detects UART pin permutations (GPIO 32/4 and 4/32) and issues multi-target initialization commands (`0x0090`).
- **Native Unit Testing Suite**: Host-compilable test framework (`run_tests.exe`) covering mathematical filters, radar payload parsing, transient dropouts, and zone boundaries.

---

## System Architecture

The firmware uses a modular task-driven architecture leveraging FreeRTOS on the ESP32.

```
+-------------------------------------------------------------------+
|                     ESP32 FreeRTOS Dual Core                      |
+-----------------------------------+-------------------------------+
| Core 0: Radar UART Task (100Hz)   | Core 1: Main UI & Web Loop    |
| - High-speed UART Byte Parser     | - TFT_eSPI Graphics Engine    |
| - E54 Frame Sync (AA FF 03 00)    | - Polar Grid & Range Scale    |
| - Nearest-Neighbor Association    | - Async Web Server Streaming  |
| - Motion Compensation Filter      | - Rotary Encoder Handling     |
+-----------------------------------+-------------------------------+
                                    | Thread-Safe Data Sync (Mutex) |
                                    +-------------------------------+
```

---

## Hardware Requirements & Pinout

### Required Components
- ESP32-WROVER-IE or ESP32-DevKitC development board.
- EBYTE E54-24LD12D 24GHz Radar Module.
- 240x320 SPI TFT Display (ST7789 or ILI9341 controller).
- EC11 Quadrature Rotary Encoder with push button.

### Pin Connections

| Function | ESP32 GPIO | Connected Device Pin | Notes |
|---|---|---|---|
| Radar RX | GPIO 32 | E54 Radar TX | Baud: 256,000 bps (Auto-detected) |
| Radar TX | GPIO 4 | E54 Radar RX | Baud: 256,000 bps (Auto-detected) |
| Encoder Channel A | GPIO 25 | Rotary Encoder CLK | Hardware pullup enabled |
| Encoder Channel B | GPIO 26 | Rotary Encoder DT | Hardware pullup enabled |
| Encoder Push | GPIO 27 | Rotary Encoder SW | Active-low |
| Secondary Key | GPIO 34 | Tactile Button KEY0 | Input-only GPIO with external pullup |
| TFT Display CS | GPIO 15 | TFT CS | SPI Chip Select |
| TFT Display DC | GPIO 2 | TFT DC / RS | Data / Command Select |
| TFT Display RST | GPIO 4 | TFT Reset | Shared or dedicated reset |

---

## Communication Protocol & Data Parsing

The EBYTE E54-24LD12D radar module outputs telemetry at 10Hz to 20Hz over serial UART (8 data bits, 1 stop bit, no parity).

### Frame Structure

```
+--------------+------------------+------------------+------------------+------------+
| Header (4B)  | Target 1 (8B)    | Target 2 (8B)    | Target 3 (8B)    | Tail (2B)  |
+--------------+------------------+------------------+------------------+------------+
| AA FF 03 00  | X  Y  Spd  Res   | X  Y  Spd  Res   | X  Y  Spd  Res   | 55 CC      |
+--------------+------------------+------------------+------------------+------------+
```

### Target Data Format (8 Bytes per Target)

Each target block contains 4 little-endian 16-bit fields:

- **X Coordinate (int16)**: Horizontal distance in millimeters. High bit (bit 15) indicates positive (1) or negative (0) direction.
- **Y Coordinate (int16)**: Forward distance in millimeters. High bit (bit 15) indicates positive (1) or negative (0) direction.
- **Speed (int16)**: Radial velocity in cm/s. High bit (bit 15) indicates positive (1) or negative (0) direction.
- **Resolution (uint16)**: Distance resolution value in millimeters.

### Initialization Commands

To switch the radar to multi-target mode on boot, the ESP32 transmits the `0x0090` command frame:

```
FD FC FB FA 02 00 90 00 04 03 02 01
```

---

## Firmware Configuration & Build System

The firmware is built using PlatformIO.

### Prerequisites
- VS Code with PlatformIO Extension or PlatformIO Core CLI.
- GCC toolchain for native C++ testing (`g++`).

### Building Firmware
To compile the ESP32 firmware binary:

```bash
pio run -e esp32-wrover-ie
```

### Flashing Firmware
To flash the compiled firmware binary to an attached ESP32:

```bash
pio run -e esp32-wrover-ie -t upload
```

### Serial Monitoring
To view live diagnostic log output at 115,200 baud:

```bash
pio device monitor -b 115200
```

---

## Web Dashboard & REST API

When `broadcastModeEnabled` is active, the ESP32 starts a Wi-Fi Access Point (`ESP32-Radar-Tracker`) and serves an interactive web dashboard on `http://192.168.4.1/`.

### Endpoints

#### GET `/`
- **Description**: Returns the embedded single-page HTML/SVG polar radar HUD dashboard.
- **Content-Type**: `text/html`

#### GET `/api/data`
- **Description**: Returns live target telemetry in JSON format.
- **Content-Type**: `application/json`
- **CORS**: `Access-Control-Allow-Origin: *`

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

## Native Testing & Verification

The project includes a host-native C++ unit test suite that executes independently of ESP32 hardware.

### Compiling and Running Tests

```bash
g++ -I src test/test_radar.cpp -o test/run_tests.exe
.\test\run_tests.exe
```

### Test Coverage
- **MotionCompensation**: Alpha-Beta-Gamma filter convergence, Nearest-Neighbor track association, transient dropout coasting, and anchor validation logic.
- **ZoneManager**: Distance/angle boundary checks and target threat level classification.
- **PerformanceMonitor**: Execution timing metrics and memory tracking.
- **E54_Radar**: Bitwise payload parsing, sign bit decoding, frame delta timing, and 3-target mock frame injection.

---

## File Structure

```
c:/Projects/E54/EBYTE_E54/
├── platformio.ini         # PlatformIO build configuration
├── src/
│   ├── main.cpp           # System entry point, FreeRTOS tasks, interrupt handlers
│   ├── E54_Radar.h        # UART byte stream parser & frame sync
│   ├── MotionCompensation.h# Alpha-Beta-Gamma filter & Nearest-Neighbor tracking
│   ├── UIManager.h        # Display rendering engine, polar grid, menu overlay
│   ├── BroadcastServer.h  # Async Web Server header
│   ├── BroadcastServer.cpp# Wi-Fi SoftAP, REST API & HTML/SVG Web Dashboard
│   ├── ConfigManager.h    # Preferences NVS storage manager
│   ├── PerformanceMonitor.h# Loop frequency & execution time metrics
│   ├── ZoneManager.h      # Spatial boundary alert manager
│   ├── Themes.h           # Color theme structure definitions
│   └── Themes.cpp         # Palette implementations (5 themes)
├── test/
│   ├── test_main.cpp      # Native test suite entry point
│   ├── test_radar.cpp     # Mock frame injection & tracking tests
│   └── test_zone_manager.cpp # Spatial boundary unit tests
└── README.md              # Project documentation
```

---

## License

This project is open-source under the MIT License.
