#pragma once

#include "movableObject.h"
#include "board.h"

enum class RotateDirection {
    Left_1_8 = 45,     // Rotate 1/8 of a circle to the left (45 degrees)
    Right_1_8 = -45,    // Rotate 1/8 of a circle to the right (45 degrees)
    Left_1_4 = 90,     // Rotate 1/4 of a circle to the left (90 degrees)
    Right_1_4 = -90     // Rotate 1/4 of a circle to the right (90 degrees)
};

class Tank : public MovableObject {
private:
    int artilleryShells;
    int shoot_counter;
    bool in_reverse;
    int reverse_counter;

public:
    Tank(int x, int y, Direction dir, Board* board);

    void shoot();
    void move_backward();
    void rotate(RotateDirection rotateDirection);

};


