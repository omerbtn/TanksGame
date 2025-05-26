#pragma once

#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include "BattleInfo.h"
#include "cell.h"


class SmartBattleInfo : public BattleInfo
{
public:
    SmartBattleInfo(const std::vector<std::vector<Cell>>& grid, Position tank_position, size_t max_steps, size_t num_shells,
                    const std::unordered_map<Position, std::unordered_set<Direction>>& shell_possible_directions = {})
        : grid_(grid), tank_position_(tank_position), max_steps_(max_steps), num_shells_(num_shells), shell_possible_drections_(shell_possible_directions) {}

    const std::vector<std::vector<Cell>>& getGrid() const { return grid_; }
    const Position& getTankPosition() const { return tank_position_; }
    size_t getMaxSteps() const { return max_steps_; }
    size_t getNumShells() const { return num_shells_; }
    const std::unordered_map<Position, std::unordered_set<Direction>>& getShellPossibleDirections() const { return shell_possible_drections_; }

    void setTankInformation(int tank_id, Direction tank_dir)
    {
        tank_id_ = tank_id;
        tank_dir_ = tank_dir;
    }

private:
    std::vector<std::vector<Cell>> grid_;
    std::unordered_map<Position, std::unordered_set<Direction>> shell_possible_drections_;
    int tank_id_;  // To be set by the tank. Really needed? Maybe if we want the player to coordinate between his tanks
    Direction tank_dir_;  // Same as above
    Position tank_position_;
    size_t max_steps_;
    size_t num_shells_;
};