#pragma once

#include "algorithm_interface.h"

#include "board.h"
#include "tank.h"
#include "algorithm_utils.h"

class SmartAlgorithm : public AlgorithmInterface
{
public:
    TankAction decideAction(const Tank &self, const Board &board) override
    {
        // return TankAction::Shoot;  // TODO: RM !!!!!!
        int sx = self.pos.first, sy = self.pos.second;

        Tank *opponent = board.getPlayerTank(self.id() == 1 ? 2 : 1);
        if (!opponent || !opponent->isAlive())
            return TankAction::Idle;

        int ex = opponent->pos.first, ey = opponent->pos.second;

        Direction currentDir = self.dir;
        Direction targetDir = getDirectionTo(sx, sy, ex, ey);

        bool alignedHorizontally = sy == ey && ((sx < ex && currentDir == Direction::R) ||
                                                (sx > ex && currentDir == Direction::L));

        bool alignedVertically = sx == ex && ((sy < ey && currentDir == Direction::D) ||
                                              (sy > ey && currentDir == Direction::U));

        if ((alignedHorizontally || alignedVertically))
            return TankAction::Shoot;

        if (currentDir == targetDir)
            return TankAction::MoveForward;

        int currentDeg = static_cast<int>(currentDir);
        int targetDeg = static_cast<int>(targetDir);
        int delta = (targetDeg - currentDeg + 360) % 360;

        if (delta == 45)
            return TankAction::RotateRight_1_8;
        else if (delta == 90)
            return TankAction::RotateRight_1_4;
        else if (delta == 135)
            return TankAction::RotateLeft_1_4;
        else if (delta == 180)
            return TankAction::RotateLeft_1_4;
        else if (delta == 225)
            return TankAction::RotateRight_1_4;
        else if (delta == 270)
            return TankAction::RotateLeft_1_4;
        else if (delta == 315)
            return TankAction::RotateLeft_1_8;

        return TankAction::Idle;
    }
};
