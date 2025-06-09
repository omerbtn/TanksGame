#pragma once

#include "algorithm_utils.h"
#include "board.h"
#include "tank.h"

// Used mostly for tests purposes.
class SeedAlgorithm : public TankAlgorithm
{
public:
    SeedAlgorithm(const std::vector<ActionRequest>& seed) : seed_{seed} {}

    ActionRequest getAction() override
    {
        if (current_step_ < seed_.size())
        {
            return seed_[current_step_++];
        }

        return ActionRequest::DoNothing;
    }

    void updateBattleInfo(BattleInfo& info) override
    {
        // No-op for seed algorithm
        (void)info;
    }

private:
    const std::vector<ActionRequest> seed_;
    size_t current_step_ = 0;
};
