#include "movable_object.h"


namespace UserCommon_322573304_322647603
{

MovableObject::MovableObject(Direction direction) : direction_{direction} {}

Direction& MovableObject::direction()
{
    return direction_;
}

const Direction& MovableObject::direction() const
{
    return direction_;
}

} // namespace UserCommon_322573304_322647603