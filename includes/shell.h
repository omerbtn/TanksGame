#pragma once

#include "game_object_interface.h"
#include "types/direction.h"

class Shell : public GameObjectInterface
{
public:
    Shell(Direction direction);

    Direction& direction();
    const Direction& direction() const;

private:
    virtual ObjectType type() const override;

    Direction direction_;
};
