#pragma once

#include "player_base.h"

class SmartPlayer : public PlayerBase
{
public:
    SmartPlayer(int player_index, size_t x, size_t y, size_t max_steps, size_t num_shells);

    virtual ~SmartPlayer() override = default;

    virtual void updateTankWithBattleInfo(TankAlgorithm& tank, SatelliteView& satellite_view) override;

private:
    std::unordered_map<int, std::unordered_set<Position>> tanks_reserved_positions_;  // tank_id -> reserved positions
};