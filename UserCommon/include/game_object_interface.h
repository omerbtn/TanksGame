#pragma once


namespace UserCommon_322573304_322647603
{

enum class ObjectType
{
    Tank,
    Shell,
    Mine,
    Wall
};

class GameObjectInterface
{
public:
    virtual ~GameObjectInterface() = default;
    virtual ObjectType type() const = 0;
};

} // namespace UserCommon_322573304_322647603