#include "Themes.h"

Theme activeTheme = { TFT_BLACK, TFT_WHITE, TFT_RED, TFT_YELLOW, TFT_GREEN, TFT_WHITE, false, 0, false, 0 };

uint16_t themeBg = TFT_BLACK;
uint16_t themePrimary = TFT_WHITE;
uint16_t themeDanger = TFT_RED;
uint16_t themeWarning = TFT_YELLOW;
uint16_t themeSuccess = TFT_GREEN;
uint16_t themeText = TFT_WHITE;

void applyTheme(int themeIndex) {
    switch (themeIndex) {
        case 0: // Standard
            activeTheme = { TFT_BLACK, TFT_WHITE, TFT_RED, TFT_YELLOW, TFT_GREEN, TFT_WHITE, false, 0, false, 0 };
            break;
        case 1: // Alien / Phosphor Green
            activeTheme = { TFT_BLACK, 0x06DD, TFT_RED, TFT_YELLOW, 0x07E0, 0x06DD, true, 0x06DD, true, 0x06DD };
            break;
        case 2: // Minimalist
            activeTheme = { 0x0000, 0xC618, 0xF800, 0xFFE0, 0x07E0, 0xC618, false, 0, false, 0 };
            break;
        case 3: // Cyberpunk (Neon Cyan / Magenta)
            activeTheme = { 0x0808, TFT_CYAN, TFT_MAGENTA, TFT_YELLOW, 0x07E0, TFT_CYAN, false, 0, false, 0 };
            break;
        case 4: // Tactical Amber / FLIR
            activeTheme = { 0x0000, TFT_ORANGE, TFT_RED, TFT_YELLOW, 0x07E0, TFT_ORANGE, false, 0, false, 0 };
            break;
                case 5: // Synthwave (Deep Violet / Neon Pink)
            activeTheme = { 0x1805, 0xF97F, TFT_CYAN, TFT_YELLOW, 0x07E0, 0xF97F, true, 0xF97F, false, 0 };
            break;
        case 6: // Blood Red / Stealth
            activeTheme = { 0x0000, 0xA000, 0xF800, TFT_ORANGE, 0xA000, 0xF800, true, 0xF800, true, 0x5000 };
            break;
        case 7: // Arctic Blue / Frozen
            activeTheme = { 0x0004, 0x8DFF, TFT_ORANGE, TFT_YELLOW, 0x07FF, 0x8DFF, false, 0, false, 0 };
            break;
        case 8: // Matrix / Deep Hacker
            activeTheme = { 0x0000, 0x03E0, TFT_RED, TFT_YELLOW, 0x07E0, 0x03E0, true, 0x07E0, true, 0x01E0 };
            break;
        default:
            activeTheme = { TFT_BLACK, TFT_WHITE, TFT_RED, TFT_YELLOW, TFT_GREEN, TFT_WHITE, false, 0, false, 0 };
            break;
    }
    themeBg = activeTheme.bg;
    themePrimary = activeTheme.primary;
    themeDanger = activeTheme.danger;
    themeWarning = activeTheme.warning;
    themeSuccess = activeTheme.success;
    themeText = activeTheme.text;
}
