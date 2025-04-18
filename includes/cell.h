#pragma once
#include "gameObject.h"
#include "tank.h"
#include "wall.h"
#include "mine.h"
#include "shell.h"

class Cell {
protected:
    Position position;
    Shell* shell = nullptr;  // Maybe just turn all these to std::vector<GameObject*> and traverse it to resolve collisions
    Mine* mine = nullptr;    // Then add_object will just add the object to the vector, and resolve_collisions will traverse
    Tank* tank = nullptr;    // the vector and check for collisions
    Wall* wall = nullptr;

public:
    Cell(Position position, GameObject* object = nullptr); // Can know the exact type by object->getType(), no need for overloading constructors
    //Cell(int x, int y, Mine* mine);
    //Cell(int x, int y, Tank* tank);
    //Cell(int x, int y, Wall* wall);
    Position getPosition() const { return position; }
    int getX() const { return position.x; }
    int getY() const { return position.y; }
    bool add_object(GameObject* obj);
    bool is_tank() const { return tank == nullptr; }
    bool is_wall() const { return wall == nullptr; }
    bool is_shell() const { return shell == nullptr; }
    bool is_mine() const { return mine == nullptr; }
    ~Cell();

private:
    void resolve_collisions();
};