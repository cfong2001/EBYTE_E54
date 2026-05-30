#pragma once

class OneButton {
public:
    OneButton(int pin, bool activeLow, bool pullupActive) {}
    void tick() {}
    void attachClick(void (*func)()) {}
    void attachLongPressStart(void (*func)()) {}
    void attachLongPressStop(void (*func)()) {}
};
