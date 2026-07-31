## 2024-07-24 - Responsive Vertical Spacing for Multi-line Screens
**Learning:** Hardcoded Y-coordinates and background box heights cause text overlap and overflow when accessibility text scaling (`uiTextSize`) is increased.
**Action:** Always dynamically calculate bounding box heights (e.g., `numLines * lh + padding`) and Y-coordinates (e.g., `baseY + lh + offset`) based on the current line height derived from font scale (e.g., `lh = 8 * uiTextSize`).
## 2024-05-24 - Avoid hardcoded colors for UI flexibility and decouple themes
**Learning:** Hardcoding absolute colors like `TFT_WHITE` or `TFT_DARKGREY` breaks visual consistency across different UI themes, leading to unstyled bounding boxes or illegible text when the background color changes. Inlining theme-specific logic (`if (theme == THEME_ALIEN)`) within the rendering code reduces modularity and scalability.
**Action:** Use dynamic theme variables like `themePrimary` or `activeTheme.text`. For structural elements, use `sprite.alphaBlend()` to generate shades contextually, e.g. `sprite.alphaBlend(128, themePrimary, themeBg)`. Extend the base `Theme` struct to handle optional overrides (`hasSweepOverride`, `sweepOverride`) to maintain a clean separation of concerns and keep rendering code data-driven.
## 2024-07-31 - Explicit Connection Status in Web Dashboards
**Learning:** Polling-based web dashboards often leave users confused if the backend connection fails silently. Without an explicit connection indicator, a static radar view might be misinterpreted as a clear airspace rather than a dead connection.
**Action:** Always implement explicit visual connection status indicators (e.g., '● LIVE' / '● OFFLINE') tied to the async fetch state (both `.then()` and `.catch()`) to prevent user confusion during network disconnections.

## 2024-07-31 - Accessible Inline SVGs
**Learning:** Inline SVGs are opaque to screen readers by default. Furthermore, the `viewBox` attribute is case-sensitive and must be camelCased for proper rendering.
**Action:** When implementing inline SVGs, ensure the `viewBox` attribute is camelCased and include `role="img"` along with a descriptive `aria-label` to support screen readers.
