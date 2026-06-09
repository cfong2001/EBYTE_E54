💡 What
Changed the text color of the "WIPING PREFERENCES..." message in the Factory Reset confirmation screen from `themePrimary` (usually white) to `themeBg` (the background color, usually black).

🎯 Why
When rendering text over a light or brightly colored alert background (such as `themeDanger` which is light red/pink `#ffb4ab`), using bright accent colors like `themePrimary` creates severe contrast issues, resulting in nearly illegible text. Switching to `themeBg` ensures a high-contrast, "knockout" text effect that maintains legibility and follows established accessibility standards for hardware interfaces.

📸 Before/After
Before: White text on light pink/red danger background (low contrast)
After: Black (or standard dark background color) text on light pink/red danger background (high contrast)

♿ Accessibility
Improves contrast ratio significantly, ensuring critical warnings are legible under various lighting conditions and across different theme choices.
