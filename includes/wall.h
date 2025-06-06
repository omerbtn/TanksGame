#pragma once

#include <cstddef>

#include "game_object_interface.h"


class Wall : public GameObjectInterface
{
public:
    void weaken();
    bool isDestroyed() const;

private:
    virtual ObjectType type() const override;

    std::size_t hit_count = 0;
};
