#pragma once

#include "movableObject.h"
#include "board.h"

class Shell : public MovableObject 
{
public:
    Shell(Position position, Direction direction, Board* board);
    virtual void move_forward();
};

