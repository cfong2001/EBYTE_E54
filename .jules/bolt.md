
## 2026-04-25 - [Single Precision Float Math Optimization]
**Learning:** Using standard double precision math functions (`sqrt`, `sin`, `cos`, `atan2`, `abs`) implicitly casts variables back and forth to double when dealing with `float` types. For the ESP32 platform, utilizing single precision variants (`sqrtf`, `sinf`, `cosf`, `atan2f`, `fabsf`) uses the hardware FPU natively and prevents the type casting overhead.
**Action:** Default to using single precision functions (e.g. `sinf`, `sqrtf`, `fabsf`) whenever the data variables are strictly typed as `float`. Avoid implicit type promotion in constrained performance-critical paths.

## 2024-05-18 - [Optimization] ESP32 Performance Enhancements
**Learning:** For optimal ESP32 rendering with TFT_eSPI, especially on screens sized 240x240 and above, initializing DMA (`tft.initDMA()`) combined with full-frame sprites enables PSRAM usage (if available) and allows the display transfer to occur concurrently with CPU logic (`tft.pushImageDMA()`). Pinning radar read processing to Core 0 with `xTaskCreatePinnedToCore` provides uninterrupted UI and inputs on Core 1 while securing serial data perfectly.
**Action:** Always enable DMA for heavy UI processing on ESP32 and distribute time-critical serial polling and UI rendering across separate cores using FreeRTOS tasks and mutexes for safety.
