## 2026-04-25 - [EBYTE SetMode Optimization]
**Learning:** `EBYTE::SetMode` changes the hardware pins for mode configurations, requiring up to 100ms in blocking `delay()`s. Because it is called at the end of many parameter-setting functions, sequential changes result in massive redundant blocking time.
**Action:** Implemented state tracking (`_mode`). By checking if `_mode == mode` and returning early, subsequent calls that don't change the mode skip the delay. This vastly improves UI responsiveness during settings save and initialization.
