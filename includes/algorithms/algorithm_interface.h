#pragma once

#include <optional>

#include "ActionRequest.h"

class Tank;
class Board;

class AlgorithmInterface
{
public:
    virtual ActionRequest decideAction(const Tank &, const Board &) = 0;
    virtual ~AlgorithmInterface() = default;

protected:
    std::optional<ActionRequest> getEvadeActionIfShellIncoming(const Tank &tank, const Board &board, size_t shell_max_distance = 4);
};
