#include "wall.h"


void Wall::weaken()
{
    hit_count++;
}

bool Wall::isDestroyed() const
{
    return hit_count >= 2;
}

ObjectType Wall::type() const
{
    return ObjectType::Wall;
}
