#pragma once

#include "algorithm_interface.h"

#include "board.h"
#include "tank.h"
#include "algorithm_utils.h"

class AggressiveChaseAlgorithm : public AlgorithmInterface
{
public:
    TankAction decideAction(const Tank &self, const Board &board) override
    {
        int sx = self.pos.first, sy = self.pos.second;

        Tank *opponent = board.getPlayerTank(self.id() == 1 ? 2 : 1);
        if (!opponent || !opponent->isAlive())
            return TankAction::Idle;

        int ex = opponent->pos.first, ey = opponent->pos.second;

        Direction currentDir = self.dir;
        Direction targetDir = getDirectionTo(sx, sy, ex, ey);

        if (currentDir == targetDir && std::abs(sx - ex) <= 1 && std::abs(sy - ey) <= 1)
            return TankAction::Shoot;

        if (currentDir == targetDir)
            return TankAction::MoveForward;

        return TankAction::RotateRight_1_8;
    }
};