## 2026-04-25 - [Memory Leak Prevention in High-FPS Menus]
**Learning:** Initializing an array of Arduino `String` objects (e.g. `String items[24];`) inside a high-frequency display loop (like `drawMenuOverlay` called at 30Hz) causes rapid heap fragmentation and subsequent CPU stalls as the ESP32 struggles to garbage-collect and reallocate memory continuously.
**Action:** Always declare string arrays used for high-frequency UI rendering as `static` (e.g. `static String items[24];`) or elevate them to class members. This prevents continuous memory allocation/deallocation overhead, vastly improving UI responsiveness.
## 2025-04-25 - HardwareSerial Buffer Size Optimization
**Learning:** For ESP32 hardware using high-speed UART (like 256000 bps for E54 radars), the default 256-byte HardwareSerial RX buffer can easily overflow within ~10ms if `loop()` execution delays.
**Action:** Use `HardwareSerial::setRxBufferSize(1024)` before calling `begin()` when working with baud rates 115200 or higher to prevent packet loss and improve reliability.
## 2024-05-19 - Defer math.sqrt calls in display loops
**Learning:** Found a performance bottleneck where `math.sqrt()` was being called on every target inside rendering loops solely to find the minimum distance for text display. On constrained embedded hardware, evaluating `math.sqrt` inside tight loops causes measurable overhead.
**Action:** When finding the closest target distance, track `closest_dist_sq` by calculating squared distances (`x*x + y*y`) inside the loop, and apply `math.sqrt()` to the final `closest_dist_sq` outside the loop before rendering.

## 2026-04-25 - [Single Precision Float Math Optimization]
**Learning:** Using standard double precision math functions (`sqrt`, `sin`, `cos`, `atan2`, `abs`) implicitly casts variables back and forth to double when dealing with `float` types. For the ESP32 platform, utilizing single precision variants (`sqrtf`, `sinf`, `cosf`, `atan2f`, `fabsf`) uses the hardware FPU natively and prevents the type casting overhead.
**Action:** Default to using single precision functions (e.g. `sinf`, `sqrtf`, `fabsf`) whenever the data variables are strictly typed as `float`. Avoid implicit type promotion in constrained performance-critical paths.

## 2024-05-18 - [Optimization] ESP32 Performance Enhancements
**Learning:** For optimal ESP32 rendering with TFT_eSPI, especially on screens sized 240x240 and above, initializing DMA (`tft.initDMA()`) combined with full-frame sprites enables PSRAM usage (if available) and allows the display transfer to occur concurrently with CPU logic (`tft.pushImageDMA()`). Pinning radar read processing to Core 0 with `xTaskCreatePinnedToCore` provides uninterrupted UI and inputs on Core 1 while securing serial data perfectly.
**Action:** Always enable DMA for heavy UI processing on ESP32 and distribute time-critical serial polling and UI rendering across separate cores using FreeRTOS tasks and mutexes for safety.
## 2026-04-26 - [C++ Memory & Optimization]
**Learning:** Initializing objects with reference variables inside a C++ header class body can cause compiler scope and 'unqualified-id' errors (especially on PlatformIO/ESP32 using ). Dynamic String creation () in a loop directly causes excessive memory fragmentation and performance regressions on constrained memory targets like the ESP32.
**Action:** Replace  inline initializations with pointers instantiated in the constructor. Use  with static  arrays instead of  class concatenations to minimize overhead.
## 2026-04-26 - [C++ Memory & Optimization]
**Learning:** Initializing objects with reference variables inside a C++ header class body can cause compiler scope and 'unqualified-id' errors (especially on PlatformIO/ESP32 using `TFT_eSprite`). Dynamic String creation in a loop directly causes excessive memory fragmentation and performance regressions on constrained memory targets like the ESP32.
**Action:** Replace `TFT_eSprite sprite` inline initializations with pointers instantiated in the constructor. Use `snprintf` with static `char` arrays instead of `String` class concatenations to minimize overhead.
## 2026-04-26 - [C++ Preprocessor Directives]
**Learning:** Preprocessor  directives directly alter code inclusion. If  completely disables logging, it should be made optional (e.g. commented out) so developers can still opt to enable diagnostic output, rather than wholesale removal.
**Action:** Retain debug toggles as commented-out configurations inside  instead of strictly overriding them to 0.
## 2026-04-26 - [C++ Preprocessor Directives]
**Learning:** Preprocessor `#define` directives directly alter code inclusion. If `-DCORE_DEBUG_LEVEL=0` completely disables logging, it should be made optional (e.g. commented out) so developers can still opt to enable diagnostic output, rather than wholesale removal.
**Action:** Retain debug toggles as commented-out configurations inside `platformio.ini` instead of strictly overriding them to 0.

## 2024-04-26 - Prevent TFT_eSPI DMA screen corruption
**Learning:** Calling `tft.pushImageDMA()` and immediately looping back to manipulate the source sprite array without waiting causes visual tearing or garbled output because the DMA controller reads memory asynchronously.
**Action:** Always include a `tft.dmaWait()` statement immediately following the `pushImageDMA()` call (before `tft.endWrite()`) to stall the CPU explicitly until the transfer finishes.

## 2024-04-26 - Optimizing OneButton for tight loops
**Learning:** `OneButton`'s `.tick()` polling can easily miss clicks if placed in an event loop experiencing latency from calculations or delays.
**Action:** Implement `OneButton` using hardware interrupts exactly like standard rotary encoders using an `IRAM_ATTR` wrapper and `attachInterrupt(..., CHANGE)` pointing to `button.tick()`.
## 2026-04-26 - Defer math.sqrt calls in alien UI draw loops
**Learning:** The memory pattern from 2024-05-19 regarding `math.sqrt()` in rendering loops applies broadly across the CircuitPython UI implementations, specifically in tracking the closest distance for the telemetry text overlay.
**Action:** Replaced direct `math.sqrt()` distance calculations and `MAX_RANGE` checks with `dist_sq` calculations inside the loops for `code_alien_advanced.py`, `code_alien_style.py`, and `code_dual_protocol.py`, deferring the final `math.sqrt()` call until after the loop for the closest distance.
