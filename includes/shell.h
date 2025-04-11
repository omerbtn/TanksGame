#pragma once

#include "movableObject.h"
#include "board.h"

class Shell : public MovableObject {


public:
    Shell(int x, int y, Direction direction, Board* board);
    virtual void move_forward();
};

