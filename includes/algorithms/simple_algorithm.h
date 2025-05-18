#pragma once

#include "algorithms/algorithm_base.h"

#include "board_battle_info.h"

class SimpleAlgorithm : public AlgorithmBase
{
public:
    virtual ~SimpleAlgorithm() = default;
    virtual ActionRequest getActionImpl() override;
};
