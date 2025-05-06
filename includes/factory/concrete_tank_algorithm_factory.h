#pragma once

#include "common/TankAlgorithmFactory.h"

#include "simple_algorithm.h"
#include "smart_algorithm.h"

class ConcreteTankAlgorithmFactory : public TankAlgorithmFactory
{
public:
    virtual ~ConcreteTankAlgorithmFactory() = default;

    virtual std::unique_ptr<TankAlgorithm> create(int player_index, int tank_index) const override
    {
        // TODO: change to switch case
        if (player_index == 1)
        {
            return std::make_unique<SimpleAlgorithm>();
        }
        else if (player_index == 2)
        {
            return std::make_unique<SmartAlgorithm>();
        }
        else
        {
            throw std::invalid_argument("Invalid player index");
        }
    }
};
