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

    std::vector<std::shared_ptr<Tank>> ordered_tanks;

    GameInfo() = default;
    GameInfo(size_t width, size_t height, size_t max_steps, size_t num_shells, std::vector<std::shared_ptr<Tank>>&& ordered_tanks)
        : is_valid(true),
          width(width),
          height(height),
          max_steps(max_steps),
          num_shells(num_shells),
          ordered_tanks(std::move(ordered_tanks)) {
    }
};
