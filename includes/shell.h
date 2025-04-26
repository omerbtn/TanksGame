#pragma once

#include "movable_object.h"
#include "types/direction.h"

class Shell : public MovableObject {
public:
    using MovableObject::MovableObject;

private:
    virtual ObjectType type() const override;
};
