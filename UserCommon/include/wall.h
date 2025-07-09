#pragma once

#include <cstddef>

#include "game_object_interface.h"


namespace UserCommon_322573304_322647603
{

class Wall : public GameObjectInterface
{
public:
    void weaken();
    bool isDestroyed() const;

private:
    virtual ObjectType type() const override;

    std::size_t hit_count = 0;
};

} // namespace UserCommon_322573304_322647603