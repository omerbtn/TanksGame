#pragma once

#include "algorithm_interface.h"

#include "board.h"
#include "tank.h"
#include "algorithm_utils.h"

class SimpleAlgorithm : public AlgorithmInterface
{
public:
    TankAction decideAction(const Tank &tank, const Board &board) override;
};
