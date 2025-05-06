#pragma once

#include <iostream>

#include "common/Player.h"

class TankAlgorithm;
class SatelliteView;

class ConcretePlayer1 : public Player {
public:
    using Player::Player;

    virtual ~ConcretePlayer1() = default;
    virtual void updateTankWithBattleInfo(TankAlgorithm& tank, SatelliteView& satellite_view) override {
        // Implement the logic to update the tank with battle information
        // This is just a placeholder implementation
        std::cout << "Updating tank with battle info for player 2" << std::endl;
    }
};
