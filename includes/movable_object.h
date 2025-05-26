#pragma once

#include "game_object_interface.h"
#include "types/direction.h"

class MovableObject : public GameObjectInterface 
{
public:
    MovableObject(Direction direction);

    Direction& direction();
    const Direction& direction() const;

private:
    virtual ObjectType type() const override = 0;

protected:
    Direction direction_;
};
