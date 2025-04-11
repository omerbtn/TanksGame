#pragma once
#include <vector>
#include <iostream>
#include "Cell.h"
#include "Tank.h"
#include "Wall.h"
#include "Mine.h"
#include "Shell.h"

class Board {
private:
    int width;  // Width of the board
    int height; // Height of the board
    std::vector<std::vector<Cell*>> board; // 2D Vector of pointers to GameObjects
    std::vector<Shell*> shells;

public:
    Board(int width, int height) : width(width), height(height);
    ~Board();
    int getHeight() const { return height; }
    int getWidth() const { return width; }
    Cell* getCell(int x, int y);
    bool isValidPosition(int x, int y);
    void printBoard();
    void addShell(Shell* shell);
};
