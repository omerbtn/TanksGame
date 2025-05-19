#pragma once

#include "TankAlgorithm.h"

class UserAlgorithm : public TankAlgorithm {
public:
    ~UserAlgorithm() = default;

    virtual ActionRequest getAction() override;
    void updateBattleInfo(BattleInfo& ) override;
};
