#pragma once
#include "GameObject.h"
#include "Tank.h"
#include "Wall.h"
#include "Mine.h"
#include "Shell.h"

class Cell {
protected:
    int x;
    int y;
    Shell* shell = nullptr;
    Mine* mine = nullptr;
    Tank* tank = nullptr;
    Wall* wall = nullptr;

public:
    Cell(int x, int y);
    Cell(int x, int y, Mine* mine);
    Cell(int x, int y, Tank* tank);
    Cell(int x, int y, Wall* wall);
    int getX() const { return x; }
    int getY() const { return y; }
    bool add_obj(GameObject* obj);
    bool is_tank() const { return tank == nullptr; }
    bool is_wall() const { return wall == nullptr; }
    bool is_shell() const { return shell == nullptr; }
    bool is_mine() const { return mine == nullptr; }
    ~Cell();
};