#pragma once

#include "types/tank_action.h"

class Tank;
class Board;

class AlgorithmInterface
{
public:
    virtual TankAction decideAction(const Tank &, const Board &) = 0;
    virtual ~AlgorithmInterface() = default;
};