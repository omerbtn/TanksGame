#pragma once

#include <string.h>

#include "GameResult.h"
#include "Player.h"
#include "SatelliteView.h"
#include "TankAlgorithm.h"


class AbstractGameManager
{
public:
    virtual ~AbstractGameManager() {}
    virtual GameResult run(
        size_t map_width, size_t map_height,
        const SatelliteView& map, // <= a snapshot, NOT updated
        std::string map_name,
        size_t max_steps, size_t num_shells,
        Player& player1, std::string name1, Player& player2, std::string name2,
        TankAlgorithmFactory player1_tank_algo_factory,
        TankAlgorithmFactory player2_tank_algo_factory) = 0;
};

using GameManagerFactory =
    std::function<std::unique_ptr<AbstractGameManager>(bool verbose)>;
