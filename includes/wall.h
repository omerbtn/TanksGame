#pragma once

#include "game_object_interface.h"
class Wall : public GameObjectInterface
{
    int hit_count = 0;

public:
    void weaken() { hit_count++; }
    bool isDestroyed() const { return hit_count >= 2; }

private:
    virtual ObjectType type() const override { return ObjectType::Wall; }
};