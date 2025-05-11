#pragma once

/*#include <queue>
#include "algorithm_interface.h"
#include "board.h"
#include "tank.h"
#include "algorithm_utils.h"*/

#include "TankAlgorithm.h"
class SmartAlgorithm : public TankAlgorithm
{
public:
    virtual ~SmartAlgorithm() = default;
    virtual ActionRequest getAction() override {
        return ActionRequest::DoNothing;
    }

    virtual void updateBattleInfo(BattleInfo& info) override {
        return;
    }
    /*TankAction decideAction(const Tank &tank, const Board &board) override;

private:
    bool isShellInPathDangerous(const Position& pos, const Board& board);
    std::optional<TankAction> findFirstSafeActionToOpponent(const Board& board, const Position& startPos, Direction startDir, const Position& targetPos);

    std::queue<TankAction> cached_path_;
    Position cached_target_;*/
};
