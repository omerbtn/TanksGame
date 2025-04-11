#pragma once
#include "board.h"
#include "gameObject.h"

enum class Direction {
    U = 90,   // Up
    UR = 45,  // Up-Right (Diagonal)
    R =0,   // Right
    DR = 315,  // Down-Right (Diagonal)
    D = 270,   // Down
    DL = 225,  // Down-Left (Diagonal)
    L = 180,   // Left
    UL  = 135  // Up-Left (Diagonal)
};

class MovableObject : public GameObject {
protected:
    Direction direction;
    Board* board;

public:
    MovableObject(int x, int y,ObjectType type,Direction direction, Board* board);
    virtual void move_forward();
private:
    void move(Direction move_drection);
};
