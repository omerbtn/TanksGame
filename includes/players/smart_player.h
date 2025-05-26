#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "Player.h"
#include "TankAlgorithm.h"
#include "SatelliteView.h"
#include "cell.h"


class SmartPlayer : public Player
{
public:
    SmartPlayer(int player_index, size_t x, size_t y, size_t max_steps, size_t num_shells);

    virtual ~SmartPlayer() override = default;

    SmartPlayer(const SmartPlayer&) = delete; // Disable copy constructor
    SmartPlayer& operator=(const SmartPlayer&) = delete; // Disable copy assignment

    virtual void updateTankWithBattleInfo(TankAlgorithm& tank, SatelliteView& satellite_view) override;

private:
    std::vector<std::vector<Cell>> reconstructGridFromSatelliteView(const SatelliteView& satellite_view, Position& r_tank_pos);

    void updateShellPossibleDirections(const std::vector<std::vector<Cell>>& prev_grid,
                                       const std::vector<std::vector<Cell>>& curr_grid);

    int player_index_;
    size_t width_, height_;
    size_t max_steps_;
    size_t num_shells_;
    std::vector<std::vector<Cell>> grid_;  // Update every time a tank asks for battle info
    std::unordered_map<Position, std::unordered_set<Direction>> shell_possible_directions_;
    // shell_possible_directions_[pos] = set of possible directions for shell at pos
};