#pragma once
#include <vector>
#include <iostream>
#include "board.h"
#include "player.h"
#include "tankAction.h"


class Algorithm {
private:
    Board* board;  // Pointer to the game board
    Player* player;  // Pointer to the player

public:
    Algorithm(Board* board, Player* player);
    TankAction get_next_step();
};