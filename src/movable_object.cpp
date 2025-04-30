#include "movable_object.h"

MovableObject::MovableObject(Direction direction) : direction_{direction} {}

Direction& MovableObject::direction() 
{
    return direction_;
}

const Direction& MovableObject::direction() const 
{
    return direction_;
}
