#ifndef THEMES_H
#define THEMES_H

#include <stdint.h>

struct Theme {
    uint16_t bg;
    uint16_t primary;
    uint16_t danger;
    uint16_t warning;
    uint16_t success;
};

// Define some basic color constants
#define TFT_BLACK       0x0000
#define TFT_WHITE       0xFFFF
#define TFT_RED         0xF800
#define TFT_GREEN       0x07E0
#define TFT_BLUE        0x001F
#define TFT_YELLOW      0xFFE0
#define TFT_CYAN        0x07FF
#define TFT_MAGENTA     0xF81F

extern Theme activeTheme;

// Legacy variables for backward compatibility if needed, though activeTheme should be used
extern uint16_t themeBg;
extern uint16_t themePrimary;
extern uint16_t themeDanger;
extern uint16_t themeWarning;
extern uint16_t themeSuccess;

#endif
