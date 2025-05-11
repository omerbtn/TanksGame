#pragma once

#include <cstddef>
#include <vector>
#include "tank.h"

class GameInfo
{
public:
    bool is_valid = false;
    size_t width = 0;
    size_t height = 0;
    size_t max_steps = 0;
    size_t num_shells = 0;
    //std::vector<std::shared_ptr<Tank>> all_tanks;
    std::vector<std::pair<int, int>> all_tanks;  // (player_id, tank_id), order of actions execution

    GameInfo() = default;
    GameInfo(size_t width, size_t height, size_t max_steps, size_t num_shells, std::vector<std::pair<int, int>> all_tanks)
        : is_valid(true), width(width), height(height), max_steps(max_steps), num_shells(num_shells), all_tanks(all_tanks) {}
};