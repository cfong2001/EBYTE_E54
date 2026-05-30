#pragma once
#include <stdint.h>
#include <string>

#define TFT_DARKGREY 0x7BEF

class TFT_eSprite {
public:
    TFT_eSprite(void* p) {}
    void fillSprite(uint16_t color) {}
    void setTextColor(uint16_t fg, uint16_t bg) {}
    void setTextColor(uint16_t color) {}
    void setCursor(int x, int y) {}
    void print(const char* s) {}
    void print(std::string s) {}
    void print(class String s) {}
    void print(int n) {}
    void print(float f) {}
    void pushSprite(int x, int y) {}
    void setTextSize(int size) {}
    void fillRect(int x, int y, int w, int h, uint16_t color) {}
    void drawLine(int x1, int y1, int x2, int y2, uint16_t color) {}
    void drawRect(int x, int y, int w, int h, uint16_t color) {}
    void setViewport(int x, int y, int w, int h) {}
    void resetViewport() {}
    int textWidth(const char* text) { return 10; }
    int textWidth(std::string text) { return 10; }
    int textWidth(class String text) { return 10; }
    uint16_t alphaBlend(uint8_t alpha, uint16_t fgc, uint16_t bgc) { return fgc; }
    void drawRoundRect(int x, int y, int w, int h, int r, uint16_t color) {}
    void fillRoundRect(int x, int y, int w, int h, int r, uint16_t color) {}
    void drawFastHLine(int x, int y, int w, uint16_t color) {}
    void drawFastVLine(int x, int y, int h, uint16_t color) {}
    void fillCircle(int x, int y, int r, uint16_t color) {}
    void drawCircle(int x, int y, int r, uint16_t color) {}
    void drawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t color) {}
    void fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t color) {}
    void drawPixel(int x, int y, uint16_t color) {}
    bool createSprite(int w, int h) { return true; }
    void* getPointer() { return nullptr; }
    void setSwapBytes(bool b) {}

    // Add printf support
    template<typename... Args>
    void printf(const char* format, Args... args) {}
};

class TFT_eSPI {
public:
    void fillScreen(uint16_t color) {}
    void setCursor(int x, int y) {}
    void print(const char* s) {}
    int width() { return 240; }
    int height() { return 240; }
    void startWrite() {}
    void endWrite() {}
    void init() {}
    void writecommand(uint8_t c) {}
    void setRotation(uint8_t r) {}
};
