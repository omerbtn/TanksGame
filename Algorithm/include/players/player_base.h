#pragma once

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Player.h"
#include "SatelliteView.h"
#include "TankAlgorithm.h"
#include "cell.h"
#include "smart_battle_info.h"


class PlayerBase : public Player
{
public:
    PlayerBase(int player_index, size_t x, size_t y, size_t max_steps, size_t num_shells);

    virtual ~PlayerBase() override = default;

    PlayerBase(const PlayerBase&) = delete;
    PlayerBase& operator=(const PlayerBase&) = delete;

    PlayerBase(PlayerBase&&) = delete;
    PlayerBase& operator=(PlayerBase&&) = delete;

    // Implemented in the derived classes
    virtual void updateTankWithBattleInfo(TankAlgorithm& tank, SatelliteView& satellite_view) override = 0;

protected:
    void setShellsAsNew(const std::vector<std::vector<Cell>>& grid);

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
    std::vector<std::vector<Cell>> grid_; // Updates every time a tank asks for battle info
    std::unordered_map<Position, std::unordered_set<Direction>> shell_possible_directions_;
    // shell_possible_directions_[pos] = set of possible directions for shell at pos
    std::unordered_set<size_t> possible_turns_passed_; // Set of possible turns passed since the last GetBattleInfo request
};
