## 2024-07-24 - Responsive Vertical Spacing for Multi-line Screens
**Learning:** Hardcoded Y-coordinates and background box heights cause text overlap and overflow when accessibility text scaling (`uiTextSize`) is increased.
**Action:** Always dynamically calculate bounding box heights (e.g., `numLines * lh + padding`) and Y-coordinates (e.g., `baseY + lh + offset`) based on the current line height derived from font scale (e.g., `lh = 8 * uiTextSize`).
## 2024-05-24 - Avoid hardcoded colors for UI flexibility and decouple themes
**Learning:** Hardcoding absolute colors like `TFT_WHITE` or `TFT_DARKGREY` breaks visual consistency across different UI themes, leading to unstyled bounding boxes or illegible text when the background color changes. Inlining theme-specific logic (`if (theme == THEME_ALIEN)`) within the rendering code reduces modularity and scalability.
**Action:** Use dynamic theme variables like `themePrimary` or `activeTheme.text`. For structural elements, use `sprite.alphaBlend()` to generate shades contextually, e.g. `sprite.alphaBlend(128, themePrimary, themeBg)`. Extend the base `Theme` struct to handle optional overrides (`hasSweepOverride`, `sweepOverride`) to maintain a clean separation of concerns and keep rendering code data-driven.

## 2024-08-06 - Explicit Offline Status on Async Disconnects
**Learning:** Polling loops in decouple dashboards often "freeze" visually if the server crashes or disconnects silently. The `fetch` function won't update UI naturally if it fails unless you manually handle the `.catch` block and the non-200 `response.ok`. Users end up seeing old/stale targets and assume the room is clear.
**Action:** When implementing asynchronous polling endpoints (like `/api/data`), always bind a distinct UI element (e.g., '● LIVE' / '● OFFLINE') directly to the network fetch state via `!response.ok` or the `.catch` block to guarantee users know they've lost connection.
