#pragma once

#include <vector>
#include <iostream>
#include <fstream>
#include "board.h"
#include "player.h"



class GameManager {
private:
    Board* board;  // Pointer to the game board
    std::vector<Player*> players;  // Vector of players
    int rounds_left;

public:
    GameManager(int boardWidth, int boardHeight);  // TODO: Should get the filename, we don't know the dimesions of the board yet
    void createBoard(const std::string& filename); // Then we can just pass the filename to the Board constructor and remove this function
    Player* runGame();  // Run the game and return the winner
    ~GameManager();
};