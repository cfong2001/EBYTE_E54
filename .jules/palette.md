
## 2026-04-25 - [Rotary Encoder UI Flow & Flicker-Free Sprites]
**Learning:** Designing menu flows for rotary encoders requires an explicit "Edit Mode" state. Combining this with full-screen `TFT_eSprite` buffering enables live previews of visual adjustments (like theme or sensitivity) by rendering the menu on top of the active simulation without flickering.
**Action:** When implementing menus on constrained SPI displays (like ESP32/TFTs), always allocate a memory sprite buffer first (if RAM allows, ~115KB for 240x240) and use `state = STATE_MENU_EDIT` logic to allow scrolling to adjust values rather than moving the cursor.

## 2026-04-25 - [Dynamic Menu & Fuzzing Visual Feedback]
**Learning:** Hardcoding static array lengths for menus breaks easily when options expand and contract based on dependencies (e.g. selecting "Custom" zones revealing X/Y parameter dials). Alpha blending (`sprite.alphaBlend`) based on floating-point danger levels (0.0 - 1.0) creates highly intuitive, gradual threat visualization instead of binary flashing.
**Action:** Always maintain a dynamic index pointer `numItems` for UI overlay menus. For critical alerts, use mathematical thresholds derived from history arrays (fuzzing) and convert those to alpha channels to drive UI color transitions smoothly.
