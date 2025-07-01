#pragma once

#include "player_base.h"


class SmartPlayer : public PlayerBase
{
public:
    virtual ~SmartPlayer() override = default;
    SmartPlayer(int player_index, size_t x, size_t y, size_t max_steps, size_t num_shells);

    SmartPlayer(const SmartPlayer&) = delete;
    SmartPlayer& operator=(const SmartPlayer&) = delete;

    SmartPlayer(SmartPlayer&&) = delete;
    SmartPlayer& operator=(SmartPlayer&&) = delete;

    virtual void updateTankWithBattleInfo(TankAlgorithm& tank, SatelliteView& satellite_view) override;

private:
    void updateWallsDamage();
    bool isShellCloseToWall(const Position& shell_pos, Direction shell_dir, Position& r_wall_pos) const;

private:
    std::unordered_map<int, std::unordered_set<Position>> tanks_reserved_positions_; // tank_id -> reserved positions
    std::unordered_map<Position, size_t> walls_damage_;                              // Wall's position -> number of hits it has taken
    std::unordered_set<std::pair<Position, Position>> reported_shell_wall_hits_;     // (shell_pos, wall_pos)
};
