#pragma once
#include "board.h"
#include "gameObject.h"
#include "direction.h"

class MovableObject : public GameObject {
protected:
    Direction direction;
    Board* board;

public:
    MovableObject(Position position, ObjectType type, Direction direction, Board* board);
    virtual void move_forward() = 0;
    void move(bool is_forward = true);  // TODO: Probably should be private
private:
};
