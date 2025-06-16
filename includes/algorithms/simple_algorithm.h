#pragma once

#include "algorithms/algorithm_base.h"

class SimpleAlgorithm : public AlgorithmBase
{
public:
    virtual ~SimpleAlgorithm() = default;
    SimpleAlgorithm(int player_index, int tank_index);

    SimpleAlgorithm(const SimpleAlgorithm&) = delete;
    SimpleAlgorithm& operator=(const SimpleAlgorithm&) = delete;

    SimpleAlgorithm(SimpleAlgorithm&&) = delete;
    SimpleAlgorithm& operator=(SimpleAlgorithm&&) = delete;

    virtual ActionRequest getActionImpl() override;
};
