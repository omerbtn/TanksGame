#pragma once
#include <string>
#include "position.h"

enum class ObjectType {
    Tank,
    Shell,
    Mine,
    Wall
};


class GameObject {
protected:
    Position position; // Position of the object on the board
    ObjectType objType; 

public:
    GameObject(Position position, ObjectType type) : position(position), objType(type) {}

    virtual ObjectType getType() const { return objType; }

    Position getPosition() const { return position; }
    int getX() const { return position.x; }  // Probably not needed, should remove these two functions
    int getY() const { return position.y; }

    virtual ~GameObject() {}
};
