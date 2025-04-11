#pragma once
#include "gameObject.h"

class Wall : public GameObject {
protected:
    int durability;

public:
    Wall(int x, int y);
    bool hit();
};
