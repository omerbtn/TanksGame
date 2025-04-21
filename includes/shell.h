#pragma once

#include "game_object_interface.h"
#include "types/direction.h"

class Shell : public GameObjectInterface
{
public:
    Direction dir;
    Shell(Direction dir) : dir(dir) {}

private:
    virtual ObjectType type() const override { return ObjectType::Shell; }
};