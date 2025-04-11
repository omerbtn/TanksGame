#pragma once

#include <vector>
#include <iostream>
#include <fstream>
#include "Board.h"
#include "Player.h"



class GameManager {
private:
    Board* board;  // Pointer to the game board
    std::vector<Player*> players;  // Vector of players
    int rounds_left;

public:
    GameManager(int boardWidth, int boardHeight);
    Board* createBoard(const std::string& filename);
    Player* runGame();
    ~GameManager();
};