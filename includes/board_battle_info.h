#pragma once

#include <cstddef>

#include "common/BattleInfo.h"
#include "cell.h"

class BoardBattleInfo : public BattleInfo {
public:
    BoardBattleInfo(const std::vector<std::vector<Cell>>& grid, int player_id, int tank_id, Position position, Direction direction,
                    size_t max_steps, size_t num_shells)
        : grid_(grid), tank_(player_id, tank_id, position, direction, num_shells) {
        // Initialize the tank with the given position and direction
        tank_.position() = position;
        tank_.direction() = direction;
    }

    std::vector<std::vector<Cell>> get_grid() const {
        return grid_;
    }

    Tank& get_tank() {
        return tank_;
    }

private:
    std::vector<std::vector<Cell>> grid_;
    Tank tank_;
};
