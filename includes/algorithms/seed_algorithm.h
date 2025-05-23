#pragma once

#include "algorithm_interface.h"

#include "board.h"
#include "tank.h"
#include "algorithm_utils.h"

// Used mostly for tests purposes.
class SeedAlgorithm : public AlgorithmInterface 
{
public:
    SeedAlgorithm(const std::vector<TankAction>& seed) : seed_{seed} {}

    TankAction decideAction(const Tank &self, const Board &board) override 
    {
        // Only to ignore the warning about unused variables, anyway this code is just for testing
        board.forward_position(self.position(), self.direction());

        if (current_step_ < seed_.size()) 
        {
            return seed_[current_step_++];
        }

        return TankAction::Idle;
    }
private:
    const std::vector<TankAction> seed_;
    size_t current_step_ = 0;
};
