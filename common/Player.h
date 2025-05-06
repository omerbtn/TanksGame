#pragma once

#include <cstddef>

class TankAlgorithm;
class SatelliteView;

class Player {
public:
    Player(int player_index, size_t x, size_t y, size_t max_steps, size_t num_shells) {}
    virtual ~Player() {}
    virtual void updateTankWithBattleInfo(TankAlgorithm& tank, SatelliteView& satellite_view) = 0;
};
