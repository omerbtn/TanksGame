#pragma once
#include <string>
#include <vector>
#include "tank.h"
#include "tankAction.h"


class Player {
private:
    std::vector<Tank*> tanks;                // Vector of pointers to the player's tanks
    int playerNumber;       // Player's number (e.g., 1 or 2)
    Board* board;
public:
    Player(std::vector<Tank*> tanks, int playerNumber);
    TankAction get_next_step();
};