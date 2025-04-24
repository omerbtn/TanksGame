#pragma once

#include "algorithm_interface.h"

#include "board.h"
#include "tank.h"
#include "algorithm_utils.h"

class AggressiveChaseAlgorithm : public AlgorithmInterface
{
public:
    TankAction decideAction(const Tank &self, const Board& board) override
    {
        int sx = self.position().first, sy = self.position().second;

        const Tank *opponent = board.get_player_tank(self.id() == 1 ? 2 : 1);
        if (!opponent || !opponent->is_alive())
            return TankAction::Idle;

        int ex = opponent->position().first, ey = opponent->position().second;

        Direction currentDir = self.direction();
        Direction targetDir = getDirectionTo(sx, sy, ex, ey);

        if (currentDir == targetDir && std::abs(sx - ex) <= 1 && std::abs(sy - ey) <= 1)
            return TankAction::Shoot;

        if (currentDir == targetDir) {
            return TankAction::Shoot;  // TankAction::MoveForward; // TODO change back
        }

        return TankAction::RotateRight_1_8;
    }
};
