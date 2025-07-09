#pragma once

#include "player_base.h"


namespace Algorithm_322573304_322647603
{

// Bring necessary types from UserCommon
using UserCommon_322573304_322647603::Direction;
using UserCommon_322573304_322647603::Position;

// Used to be SmartPlayer
class Player_322573304_322647603 : public PlayerBase
{
public:
    virtual ~Player_322573304_322647603() override = default;
    Player_322573304_322647603(int player_index, size_t x, size_t y, size_t max_steps, size_t num_shells);

    Player_322573304_322647603(const Player_322573304_322647603&) = delete;
    Player_322573304_322647603& operator=(const Player_322573304_322647603&) = delete;

    Player_322573304_322647603(Player_322573304_322647603&&) = delete;
    Player_322573304_322647603& operator=(Player_322573304_322647603&&) = delete;

    virtual void updateTankWithBattleInfo(TankAlgorithm& tank, SatelliteView& satellite_view) override;

private:
    void updateWallsDamage();
    bool isShellCloseToWall(const Position& shell_pos, Direction shell_dir, Position& r_wall_pos) const;

private:
    std::unordered_map<int, std::unordered_set<Position>> tanks_reserved_positions_; // tank_id -> reserved positions
    std::unordered_map<Position, size_t> walls_damage_;                              // Wall's position -> number of hits it has taken
    std::unordered_set<std::pair<Position, Position>> reported_shell_wall_hits_;     // (shell_pos, wall_pos)
};

} // namespace Algorithm_322573304_322647603