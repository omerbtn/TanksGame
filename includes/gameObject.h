#pragma once
#include <string>

enum class ObjectType {
    Tank,
    Shell,
    Mine,
    Wall
};



class GameObject {
protected:
    int x; 
    int y; 
    ObjectType objType; 

public:
    GameObject(int x, int y, ObjectType type) : x(x), y(y), objType(type) {}

    virtual ObjectType getType() const { return objType; }

    int getX() const { return x; }
    int getY() const { return y; }

    virtual ~GameObject() {}
};
