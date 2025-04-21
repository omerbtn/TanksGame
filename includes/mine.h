#pragma once

#include "game_object_interface.h"

class Mine : public GameObjectInterface
{
private:
    virtual ObjectType type() const override { return ObjectType::Mine; }
};