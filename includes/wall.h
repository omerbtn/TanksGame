#pragma once
#include "gameObject.h"

class Wall : public GameObject {
protected:
    int durability;

public:
    Wall(Position position);
    bool hit();
};
