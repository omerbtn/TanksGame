#pragma once

#include "player_base.h"

class SimplePlayer : public PlayerBase
{
public:
    SimplePlayer(int player_index, size_t x, size_t y, size_t max_steps, size_t num_shells);

    virtual ~SimplePlayer() override = default;

    virtual void updateTankWithBattleInfo(TankAlgorithm& tank, SatelliteView& satellite_view) override;
};