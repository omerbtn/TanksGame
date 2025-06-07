#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "Player.h"
#include "TankAlgorithm.h"
#include "SatelliteView.h"
#include "smart_battle_info.h"
#include "cell.h"


class PlayerBase : public Player
{
public:
    PlayerBase(int player_index, size_t x, size_t y, size_t max_steps, size_t num_shells);

    virtual ~PlayerBase() override = default;

    PlayerBase(const PlayerBase&) = delete; // Disable copy constructor
    PlayerBase& operator=(const PlayerBase&) = delete; // Disable copy assignment

    // Implemented in the derived classes
    virtual void updateTankWithBattleInfo(TankAlgorithm &tank, SatelliteView &satellite_view) override = 0;

protected:
    std::vector<std::vector<Cell>> reconstructGridFromSatelliteView(const SatelliteView& satellite_view, Position& r_tank_pos);

    void setShellsAsNew(const std::vector<std::vector<Cell>> &grid);

    void updateShellPossibleDirections(const std::vector<std::vector<Cell>>& prev_grid,
                                       const std::vector<std::vector<Cell>>& curr_grid);

    void getShellPossibleDirectionsForTurnsPassed(const std::vector<std::vector<Cell>>& prev_grid,
                                                  const std::vector<std::vector<Cell>>& curr_grid,
                                                  const std::unordered_map<Position, std::unordered_set<Direction>>& prev_shell_possible_directions,
                                                  size_t turns_passed,
                                                  std::unordered_map<Position, std::unordered_set<Direction>>& candidate_map,
                                                  std::vector<Position>& unexplainable_shells);

    void accumulateDirections(std::unordered_map<Position, std::unordered_set<Direction>>& accumulated_directions,
                              std::unordered_map<Position, std::unordered_set<Direction>>& candidate_map,
                              const std::vector<Position>& unexplainable_shells);

    SmartBattleInfo createBattleInfo(const SatelliteView& satellite_view);

    int player_index_;
    size_t width_, height_;
    size_t max_steps_;
    size_t num_shells_;
    std::vector<std::vector<Cell>> grid_;  // Update every time a tank asks for battle info
    std::unordered_map<Position, std::unordered_set<Direction>> shell_possible_directions_;
    // shell_possible_directions_[pos] = set of possible directions for shell at pos
};