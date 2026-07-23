# ESP32 Radar Tracker (EBYTE E54)

An open-source, handheld 24GHz radar tracker that works like a real-world Call of Duty: Modern Warfare 2 (MW2) heartbeat sensor.

It detects moving people through walls, drywall, plastic, and total darkness, showing their position, distance, and speed on a color TFT screen in real time. It also streams live radar coordinates over Wi-Fi to smartphones and web browsers.

[![ESP32 Radar Display Demonstration](https://img.youtube.com/vi/HvI1Ko4NO3E/maxresdefault.jpg)](https://www.youtube.com/watch?v=HvI1Ko4NO3E)

*Click the image above to watch the live hardware demonstration video on YouTube.*

---

## What It Does

- **Sees Through Walls & Darkness**: Uses 24GHz millimeter-wave radar instead of cameras. It tracks movement through drywall, glass, plastic, and pitch darkness without recording video or audio.
- **MW2-Style Radar Screen**: Renders a smooth 60–120 FPS sweeping polar HUD on an attached 240x320 color TFT display.
- **Handheld Motion Compensation**: Filters out hand shake when held or walked with, using multi-target anchor tracking to separate device wobble from real target movement.
- **Tracks Up to 3 Targets**: Simultaneously tracks lateral position (X), distance (Y), and movement speed for up to three people.
- **1-Second Path Extrapolation**: If a target momentarily steps behind a thick barrier, the system predicts their movement trajectory for up to one second so the track doesn't flicker away.
- **Wi-Fi Web Dashboard**: Connect any phone or laptop to the board's built-in Wi-Fi access point (`ESP32-Radar-Tracker`) to view a live web HUD or stream JSON data into Home Assistant or custom software.

---

## Quick Hardware Specifications

| Feature | Specification |
|---|---|
| Radar Module | EBYTE E54-24LD12D (24GHz FMCW) |
| Target Capacity | Up to 3 simultaneous tracked targets |
| Detection Range | 0.5m to 30.0m (Auto-scaling 2m grid arcs) |
| Serial Baud Rate | 256,000 baud (Auto-detected on boot) |
| Display | 240x320 SPI TFT Screen (ST7789 or ILI9341) |
| Display Frame Rate | 60 FPS to 120 FPS (Direct-to-sprite rendering) |
| Wireless Telemetry | Wi-Fi 802.11 b/g/n (`ESP32-Radar-Tracker`) |
| Web API Output | Real-time JSON stream (`GET /api/data`) |
| Controls | EC11 Rotary Encoder (100ms debounced) & Tactile Buttons |
| Power | 5V DC via USB-C or external 5V pin |

---

## Hardware Wiring

### Pin Connections

| Function | ESP32 GPIO | Connected Device | Notes |
|---|---|---|---|
| Radar RX | GPIO 32 | E54 Radar TX | Serial input (Auto-detected with GPIO 4) |
| Radar TX | GPIO 4 | E54 Radar RX | Serial configuration output |
| Encoder CLK | GPIO 25 | Encoder Channel A | Internal pull-up enabled |
| Encoder DT | GPIO 26 | Encoder Channel B | Internal pull-up enabled |
| Encoder Push | GPIO 27 | Encoder Button | Click button (100ms debounced) |
| Tactile Key | GPIO 34 | Button KEY0 | Menu navigation |
| Display CS | GPIO 15 | TFT CS | SPI Chip Select |
| Display DC | GPIO 2 | TFT DC / RS | Data / Command Control |
| Display Reset | GPIO 4 | TFT RST | Panel Reset |

---

## Web API & JSON Stream

Connect your phone, tablet, or PC to the `ESP32-Radar-Tracker` Wi-Fi network.

- **`GET /`**: Opens the interactive web radar dashboard.
- **`GET /api/data`**: Returns live coordinate JSON data for home automation or custom apps.

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

## Build & Flash Instructions

Built using [PlatformIO](https://platformio.org/).

### Compile Firmware
```bash
pio run -e esp32-wrover-ie
```

### Flash to ESP32
```bash
pio run -e esp32-wrover-ie -t upload
```

### Serial Monitor
```bash
pio device monitor -b 115200
```

---

## Offline C++ Unit Testing

You can run the full signal processing, parser, and zone test suite on your computer without hardware connected.

### Windows (PowerShell / CMD)
```powershell
g++ -std=c++11 -I test/include -I src test/test_main.cpp test/test_mocks.cpp test/test_motion_compensation.cpp test/test_performance_monitor.cpp test/test_radar.cpp test/test_zone_manager.cpp -o test/run_tests.exe
.\test\run_tests.exe
```

### Linux / macOS
```bash
make -C test test
```

---

## Project Structure

```
EBYTE_E54/
├── .github/workflows/      # GitHub Actions CI build workflow
├── boards/                  # Custom board manifest
├── src/                    # ESP32 C++ firmware source
│   ├── main.cpp            # FreeRTOS setup & main loops
│   ├── E54_Radar.h         # E54 radar frame parser
│   ├── MotionCompensation.h# Motion compensation & tracking logic
│   ├── UIManager.h         # TFT screen HUD & vector rendering
│   ├── ZoneManager.h       # Warning & dead zone evaluator
│   ├── Themes.h / .cpp     # 5 visual color themes
│   ├── ConfigManager.h     # NVS settings storage
│   ├── PerformanceMonitor.h# Metric timing tracker
│   └── BroadcastServer.h/cpp # Wi-Fi web server & REST API
├── test/                   # Host-native C++ unit test suite
├── wokwi/                  # Wokwi web simulator setup
├── LICENSE                 # License file (CC BY-NC-SA 4.0)
├── platformio.ini          # PlatformIO build settings
└── README.md               # Project documentation
```

---

## License

This project is licensed under **Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)**.

- **Non-Commercial**: Free for open-source community, personal, and educational use. Commercial use or sales require explicit permission.
- **Share-Alike**: All modifications and derivative projects must be shared under the exact same CC BY-NC-SA 4.0 license terms.

See [LICENSE](LICENSE) for details.
