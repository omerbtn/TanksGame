#pragma once

#include "player_base.h"


class SimplePlayer : public PlayerBase
{
public:
    virtual ~SimplePlayer() override = default;
    SimplePlayer(int player_index, size_t x, size_t y, size_t max_steps, size_t num_shells);

    SimplePlayer(const SimplePlayer&) = delete;
    SimplePlayer& operator=(const SimplePlayer&) = delete;

    SimplePlayer(SimplePlayer&&) = delete;
    SimplePlayer& operator=(SimplePlayer&&) = delete;

    virtual void updateTankWithBattleInfo(TankAlgorithm& tank, SatelliteView& satellite_view) override;
};
