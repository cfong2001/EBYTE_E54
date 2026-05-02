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
## 2024-04-26 - ESP32 Dual-Core FreeRTOS Performance Profiling
**Learning:** Measuring ESP32 FreeRTOS core execution and idle times using `uxTaskGetSystemState` is extremely powerful for diagnosing execution bottlenecks but causes C++ compilation errors (`undefined reference`) on default ESP32 toolchains unless custom ESP-IDF SDK configuration (`CONFIG_FREERTOS_USE_TRACE_FACILITY`) is explicitly compiled into the underlying framework core.
**Action:** When profiling standard Arduino-ESP32 setups, fallback to utilizing standard FreeRTOS APIs like `uxTaskGetNumberOfTasks()` and `ESP.getFreeHeap()` / `ESP.getMaxAllocHeap()` embedded in a low-priority modular background task (`xTaskCreatePinnedToCore`) rather than relying on advanced trace metrics unless the framework is explicitly built with trace support.

## 2026-05-02 - [Precalculate Trigonometric Functions in Python UI Loops]
**Learning:** Repetitive calculation of `math.cos` and `math.sin` combined with `math.radians` inside UI rendering loops (e.g. iterating over angles to draw circles or arcs) causes significant CPU overhead in Python, especially on constrained environments like CircuitPython on ESP32.
**Action:** Precalculate fixed coordinate offsets or trigonometric values into static module-level tuples (e.g., `COS_TABLE` and `SIN_TABLE` mapping degrees 0-359 to their float values). Use integer modulo arithmetic `angle % 360` to perform rapid table lookups instead of calculating trig functions on the fly during rendering.
