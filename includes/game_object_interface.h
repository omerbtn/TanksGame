#pragma once

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