#pragma once

/*#include <queue>
#include "algorithm_interface.h"
#include "board.h"
#include "tank.h"
#include "algorithm_utils.h"*/

#include <queue>

#include "algorithms/algorithm_base.h"
#include "board_battle_info.h"
#include "algorithm_utils.h"

class SmartAlgorithm : public AlgorithmBase {
public:
    virtual ~SmartAlgorithm() = default;
    SmartAlgorithm(int player_index, int tank_index);

    virtual ActionRequest getActionImpl() override;

private:
    bool isShellInPathDangerous(const Position& pos);
    std::optional<ActionRequest> findFirstSafeActionToOpponent(const Position& startPos, Direction startDir);

    std::queue<ActionRequest> cached_path_;
    Position cached_target_;
    int player_index_;
    int tank_index_; // TODO: maybe remove..
};
