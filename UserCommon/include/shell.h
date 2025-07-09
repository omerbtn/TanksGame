#pragma once

#include "movable_object.h"
#include "types/direction.h"


namespace UserCommon_322573304_322647603
{

class Shell : public MovableObject
{
public:
    using MovableObject::MovableObject;

private:
    virtual ObjectType type() const override;
};

} // namespace UserCommon_322573304_322647603