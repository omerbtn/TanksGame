#pragma once

#include "movableObject.h"

class Shell : public MovableObject 
{
public:
    Shell(Position position, Direction direction, Board* board);
    virtual void move_forward() override;
};

