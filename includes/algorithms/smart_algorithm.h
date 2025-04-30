#pragma once

#include <queue>
#include "algorithm_interface.h"
#include "board.h"
#include "tank.h"
#include "algorithm_utils.h"

class SmartAlgorithm : public AlgorithmInterface
{
public:
    TankAction decideAction(const Tank &tank, const Board &board) override;

private:
    bool isShellInPathDangerous(const Position& pos, const Board& board);
    std::optional<TankAction> findFirstSafeActionToOpponent(const Board& board, const Position& startPos, Direction startDir, const Position& targetPos);
    
    std::queue<TankAction> cached_path_;
    Position cached_target_;
};