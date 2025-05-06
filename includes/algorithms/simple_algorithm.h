#pragma once

#include "common/TankAlgorithm.h"

class SimpleAlgorithm : public TankAlgorithm
{
public:
    //TankAction decideAction(const Tank&tank, const Board &board) override;
    virtual ~SimpleAlgorithm() = default;
    virtual ActionRequest getAction() override {
        return ActionRequest::DoNothing;
    }

    virtual void updateBattleInfo(BattleInfo& info) override {
        (void)info;
        return;
    }
};
