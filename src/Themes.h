#ifndef THEMES_H
#define THEMES_H

#include <stdint.h>

struct Theme {
    uint16_t bg;
    uint16_t primary;
    uint16_t danger;
    uint16_t warning;
    uint16_t success;
    uint16_t text;
    bool hasSweepOverride;
    uint16_t sweepOverride;
    bool hasGridOverride;
    uint16_t gridOverride;
};

// Define basic 16-bit RGB565 color constants
#define TFT_BLACK       0x0000
#define TFT_WHITE       0xFFFF
#define TFT_RED         0xF800
#define TFT_GREEN       0x07E0
#define TFT_BLUE        0x001F
#define TFT_YELLOW      0xFFE0
#define TFT_CYAN        0x07FF
#define TFT_MAGENTA     0xF81F
#define TFT_ORANGE      0xFDA0
#define TFT_PURPLE      0x780F

extern Theme activeTheme;

extern uint16_t themeBg;
extern uint16_t themePrimary;
extern uint16_t themeDanger;
extern uint16_t themeWarning;
extern uint16_t themeSuccess;
extern uint16_t themeText;

void applyTheme(int themeIndex);

#endif
