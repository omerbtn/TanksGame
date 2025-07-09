#include "wall.h"


namespace UserCommon_322573304_322647603
{

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

} // namespace UserCommon_322573304_322647603