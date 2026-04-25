
## 2026-04-25 - [Rotary Encoder UI Flow & Flicker-Free Sprites]
**Learning:** Designing menu flows for rotary encoders requires an explicit "Edit Mode" state. Combining this with full-screen `TFT_eSprite` buffering enables live previews of visual adjustments (like theme or sensitivity) by rendering the menu on top of the active simulation without flickering.
**Action:** When implementing menus on constrained SPI displays (like ESP32/TFTs), always allocate a memory sprite buffer first (if RAM allows, ~115KB for 240x240) and use `state = STATE_MENU_EDIT` logic to allow scrolling to adjust values rather than moving the cursor.
