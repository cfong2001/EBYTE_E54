#ifndef THEMES_H
#define THEMES_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <ArduinoJson.h>

struct Theme {
    String name;
    uint16_t bg;
    uint16_t primary;
    uint16_t danger;
    uint16_t success;
    uint16_t warning;
    uint16_t text;
};

// Fallback theme directly in code
const Theme FALLBACK_THEME = {"Alien", 0x1082, 0x06DD, 0xFDB5, 0x2F20, 0xFDCA, 0x06DD};

// Global themes array, initially just holding the fallback
extern Theme userThemes[10];
extern int numUserThemes;

#endif // THEMES_H
