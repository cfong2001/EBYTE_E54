
## 2026-04-25 - [Rotary Encoder UI Flow & Flicker-Free Sprites]
**Learning:** Designing menu flows for rotary encoders requires an explicit "Edit Mode" state. Combining this with full-screen `TFT_eSprite` buffering enables live previews of visual adjustments (like theme or sensitivity) by rendering the menu on top of the active simulation without flickering.
**Action:** When implementing menus on constrained SPI displays (like ESP32/TFTs), always allocate a memory sprite buffer first (if RAM allows, ~115KB for 240x240) and use `state = STATE_MENU_EDIT` logic to allow scrolling to adjust values rather than moving the cursor.

## 2026-04-25 - [Dynamic Menu & Fuzzing Visual Feedback]
**Learning:** Hardcoding static array lengths for menus breaks easily when options expand and contract based on dependencies (e.g. selecting "Custom" zones revealing X/Y parameter dials). Alpha blending (`sprite.alphaBlend`) based on floating-point danger levels (0.0 - 1.0) creates highly intuitive, gradual threat visualization instead of binary flashing.
**Action:** Always maintain a dynamic index pointer `numItems` for UI overlay menus. For critical alerts, use mathematical thresholds derived from history arrays (fuzzing) and convert those to alpha channels to drive UI color transitions smoothly.

## 2026-04-25 - [Tactical Micro-UX & Persistence]
**Learning:** Hard-coded variables in complex UIs frustrate users if lost on power-cycle. Transitioning to NVS using `Preferences.h` is critical for hardware tools. Additionally, small UX details like target color-coding (Orange, Blue, Purple vs Monochromatic) drastically reduce cognitive load, and replacing static text boot screens with animated progressive line-drawing mimics high-end instrumentation.
**Action:** Default to implementing NVS `saveSettings()` and `loadSettings()` for any user-facing configuration structs. Use distinct colors per tracked ID rather than per-theme unless monochrome is strictly requested.

## 2026-04-25 - [Data Telemetry & Simulated Sweeps]
**Learning:** Displaying data requires distinct user intentions. Sometimes users want raw radar coordinates, sometimes derived distance/angle, and sometimes just velocity. Categorizing these into an enum (`TelemetryMode`) in a "Target Data" sub-menu provides immense utility. Furthermore, calculating simulated sweeps by matching the hardware's reported physical angle to the UI's sweep angle provides a highly authentic "legacy radar" mode.
**Action:** When implementing readouts, avoid reversing display pixels back into world units; intercept and cache the raw telemetry data (`rawTargetX`, `rawTargetSpeed`) before it hits the UI layer. Group dense settings into `MenuPage` hierarchies.
