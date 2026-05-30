#pragma once

#define TFT_BLACK       0x0000
#define TFT_WHITE       0xFFFF
#define TFT_DARKGREY    0x7BEF

class TFT_eSprite {
public:
    void fillScreen(uint32_t color) {}
    void startWrite() {}
    void endWrite() {}
    void drawString(const char* s, int x, int y) {}
    void drawString(const String& s, int x, int y) {}
    void setTextColor(uint32_t fg, uint32_t bg) {}
    void setTextColor(uint32_t fg) {}
    void setTextDatum(uint8_t datum) {}
    int16_t textWidth(const char* s) { return 0; }
    int16_t textWidth(const String& s) { return 0; }
    uint16_t alphaBlend(uint8_t alpha, uint16_t fgc, uint16_t bgc) { return fgc; }
    void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {}
    void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {}
    void fillSprite(uint32_t color) {}
    void setCursor(int16_t x, int16_t y) {}
    void print(const char* s) {}

    void drawCircle(int x, int y, int r, uint32_t c) {}
    void fillCircle(int x, int y, int r, uint32_t c) {}
    void drawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint32_t c) {}
    void fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint32_t c) {}
    void drawRoundRect(int x, int y, int w, int h, int r, uint32_t c) {}
    void fillRoundRect(int x, int y, int w, int h, int r, uint32_t c) {}
    void print(const String& s) {}


    void printf(const char* format, ...) {}
    void drawPixel(int x, int y, uint32_t color) {}
    void drawLine(int x1, int y1, int x2, int y2, uint32_t color) {}
    void setTextSize(uint8_t size) {}
    void setViewport(int x, int y, int w, int h) {}
    void resetViewport() {}

    void pushSprite(int32_t x, int32_t y) {}
};

class TFT_eSPI {
public:
    int16_t width() { return 240; }
    int16_t height() { return 240; }
    void fillScreen(uint32_t color) {}
    void startWrite() {}
    void endWrite() {}
    void drawString(const char* s, int x, int y) {}
    void drawString(const String& s, int x, int y) {}
    void setTextColor(uint32_t fg, uint32_t bg) {}
    void setTextColor(uint32_t fg) {}
    void setTextDatum(uint8_t datum) {}
    int16_t textWidth(const char* s) { return 0; }
    int16_t textWidth(const String& s) { return 0; }
};
