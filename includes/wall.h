#pragma once

#include <cstddef>

#include "game_object_interface.h"

class Wall : public GameObjectInterface
{
public:
    void weaken();
    bool is_destroyed() const;

private:
    virtual ObjectType type() const override;

    std::size_t hit_count = 0;
};
