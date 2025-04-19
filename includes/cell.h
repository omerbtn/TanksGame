#pragma once

#include "Position.h"

class GameObject;
class Shell;
class Mine;
class Tank;
class Wall;

class Cell {
protected:
    Position position;
    Shell* shell = nullptr;  // Maybe just turn all these to std::vector<GameObject*> and traverse it to resolve collisions
    Mine* mine = nullptr;    // Then add_object will just add the object to the vector, and resolve_collisions will traverse
    Tank* tank = nullptr;    // the vector and check for collisions
    Wall* wall = nullptr;

public:
    explicit Cell(Position position, GameObject* object = nullptr); // Can know the exact type by object->getType(), no need for overloading constructors
    //Cell(int x, int y, Mine* mine);
    //Cell(int x, int y, Tank* tank);
    //Cell(int x, int y, Wall* wall);
    Position getPosition() const;
    int getX() const;
    int getY() const;
    bool add_object(GameObject* obj);
    bool is_tank() const;
    bool is_wall() const;
    bool is_shell() const;
    bool is_mine() const;
    ~Cell() = default;

private:
    void resolve_collisions();
};