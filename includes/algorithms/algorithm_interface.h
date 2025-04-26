#pragma once

#include <optional>

#include "types/tank_action.h"

class Tank;
class Board;

class AlgorithmInterface
{
public:
    virtual TankAction decideAction(const Tank &, const Board &) = 0;
    virtual ~AlgorithmInterface() = default;

protected:
    std::optional<TankAction> getEvadeActionIfShellIncoming(const Tank &tank, const Board &board);
};
