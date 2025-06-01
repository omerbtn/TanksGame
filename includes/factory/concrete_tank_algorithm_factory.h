#pragma once

#include <cassert>

#include "TankAlgorithmFactory.h"

#include "simple_algorithm.h"
#include "smart_algorithm.h"
#include "seed_algorithm.h"
#include "user_algorithm.h"

class ConcreteTankAlgorithmFactory : public TankAlgorithmFactory
{
public:
    virtual ~ConcreteTankAlgorithmFactory() = default;

    virtual std::unique_ptr<TankAlgorithm> create(int player_index, int tank_index) const override
    {
        if (player_index > 9 || player_index < 1)
        {
            throw std::invalid_argument("Invalid player index");
        }

        switch (player_index) {
            case 1:
                // return std::make_unique<SmartAlgorithm>(player_index, tank_index);  // TODO: Switch back
                return std::make_unique<UserAlgorithm>(player_index, tank_index);
            case 2:
                // return std::make_unique<SimpleAlgorithm>(player_index, tank_index);
                return std::make_unique<UserAlgorithm>(player_index, tank_index);
            case 3:
                return std::make_unique<UserAlgorithm>(player_index, tank_index);
            case 4: [[fallthrough]];
            case 5: [[fallthrough]];
            case 6: [[fallthrough]];
            case 7: [[fallthrough]];
            case 8: [[fallthrough]];
            case 9:
                return std::make_unique<SmartAlgorithm>(player_index, tank_index);
            default:
                assert(false);
                throw std::invalid_argument("Invalid player index");

        }
    }
};
