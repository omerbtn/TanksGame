#pragma once

#include <cassert>

#include "TankAlgorithmFactory.h"
#include "seed_algorithm.h"
#include "simple_algorithm.h"
#include "smart_algorithm.h"
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

        if (player_index % 2 == 0)
        {
            return std::make_unique<SimpleAlgorithm>(player_index, tank_index);
        }
        else
        {
            return std::make_unique<SmartAlgorithm>(player_index, tank_index);
        }
    }
};
