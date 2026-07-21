#include "Themes.h"

Theme activeTheme = { TFT_BLACK, TFT_WHITE, TFT_RED, TFT_YELLOW, TFT_GREEN, TFT_WHITE };

uint16_t themeBg = TFT_BLACK;
uint16_t themePrimary = TFT_WHITE;
uint16_t themeDanger = TFT_RED;
uint16_t themeWarning = TFT_YELLOW;
uint16_t themeSuccess = TFT_GREEN;
uint16_t themeText = TFT_WHITE;

void applyTheme(int themeIndex) {
    switch (themeIndex) {
        case 0: // Standard
            activeTheme = { TFT_BLACK, TFT_WHITE, TFT_RED, TFT_YELLOW, TFT_GREEN, TFT_WHITE };
            break;
        case 1: // Alien / Phosphor Green
            activeTheme = { TFT_BLACK, 0x06DD, TFT_RED, TFT_YELLOW, 0x07E0, 0x06DD };
            break;
        case 2: // Minimalist
            activeTheme = { 0x0000, 0xC618, 0xF800, 0xFFE0, 0x07E0, 0xC618 };
            break;
        case 3: // Cyberpunk (Neon Cyan / Magenta)
            activeTheme = { 0x0808, TFT_CYAN, TFT_MAGENTA, TFT_YELLOW, 0x07E0, TFT_CYAN };
            break;
        case 4: // Tactical Amber / FLIR
            activeTheme = { 0x0000, TFT_ORANGE, TFT_RED, TFT_YELLOW, 0x07E0, TFT_ORANGE };
            break;
        default:
            activeTheme = { TFT_BLACK, TFT_WHITE, TFT_RED, TFT_YELLOW, TFT_GREEN, TFT_WHITE };
            break;
    }
    themeBg = activeTheme.bg;
    themePrimary = activeTheme.primary;
    themeDanger = activeTheme.danger;
    themeWarning = activeTheme.warning;
    themeSuccess = activeTheme.success;
    themeText = activeTheme.text;
}
