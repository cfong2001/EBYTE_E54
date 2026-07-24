## 2024-07-24 - Responsive Vertical Spacing for Multi-line Screens
**Learning:** Hardcoded Y-coordinates and background box heights cause text overlap and overflow when accessibility text scaling (`uiTextSize`) is increased.
**Action:** Always dynamically calculate bounding box heights (e.g., `numLines * lh + padding`) and Y-coordinates (e.g., `baseY + lh + offset`) based on the current line height derived from font scale (e.g., `lh = 8 * uiTextSize`).
