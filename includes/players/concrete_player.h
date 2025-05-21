#pragma once

#include <iostream>

#include "Player.h"
#include "TankAlgorithm.h"
#include "SatelliteView.h"

#include "position.h"
#include "direction.h"
#include "board_battle_info.h"
#include "player_utils.h"
#include "algorithm_utils.h"

template <int PlayerIndex>
class ConcretePlayer : public Player {
public:
    virtual ~ConcretePlayer() override = default;

    ConcretePlayer(size_t x, size_t y, size_t max_steps, size_t num_shells)
        : Player(PlayerIndex, x, y, max_steps, num_shells), x_(x), y_(y), maxSteps_(max_steps), numShells_(num_shells) {}
        
    ConcretePlayer(const ConcretePlayer&) = delete; // Disable copy constructor
    ConcretePlayer& operator=(const ConcretePlayer&) = delete; // Disable copy assignment

    virtual void updateTankWithBattleInfo(TankAlgorithm& tank, SatelliteView& satellite_view) override 
    {
        Position tank_position;
        auto grid = reconstruct_grid_from_satellite_view(satellite_view, PlayerIndex, tank_position);
        Direction dir = getSeedDirection(PlayerIndex);

        BoardBattleInfo info(grid, PlayerIndex, tank_id_, tank_position, dir, maxSteps_, numShells_);
        tank.updateBattleInfo(info);
    }

private:
    size_t x_, y_;
    size_t maxSteps_;
    size_t numShells_;
    size_t tank_id_ = 0;
};
