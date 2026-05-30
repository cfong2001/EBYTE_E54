#pragma once

class RotaryEncoder {
public:
    enum class Direction { NOROTATION = 0, CLOCKWISE = 1, COUNTERCLOCKWISE = -1 };

    RotaryEncoder(int pin1, int pin2, void* type) {}
    void tick() {}
    Direction getDirection() { return Direction::NOROTATION; }
};
