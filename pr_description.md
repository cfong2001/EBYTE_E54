💡 **What:** Replaced hardcoded Y-coordinates with dynamically incremented variables in `drawSelfTestScreen`.
🎯 **Why:** To prevent trailing UI text elements (like "Click to exit") from jittering or shifting vertically as the screen transitions from the active loading state (`RUNNING TESTS...`) to the completed results state (`PASS`/`FAIL`). Using a consistently incremented variable guarantees stable rendering across conditional branches.
📸 **Before/After:** The self-test text now renders smoothly without visual jumping when tests conclude.
♿ **Accessibility:** This also properly scales the text blocks vertically depending on the user's `uiTextSize` preference, avoiding overlap at larger scales.
