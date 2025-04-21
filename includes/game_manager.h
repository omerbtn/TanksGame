#pragma once

#include "board.h"
#include "tank.h"
#include "output_logger.h"
#include "algorithms/algorithm_interface.h"

class GameManager
{
public:
    GameManager(Board *board, Tank *tank1, Tank *tank2, AlgorithmInterface *algo1, AlgorithmInterface *algo2)
        : board_(board), tank1_(tank1), tank2_(tank2), algo1_(algo1), algo2_(algo2), step_count_(0), tie_countdown_(40) {}

    void run()
    {
        OutputLogger logger("output.txt");
        while (!isGameOver())
        {
            step(logger);
            board_->print();
        }
        logger.logResult(*tank1_, *tank2_, step_count_);
    }

private:
    Board *board_;
    Tank *tank1_; // TODO add player class and inside the tank + algo
    Tank *tank2_;
    AlgorithmInterface *algo1_;
    AlgorithmInterface *algo2_;
    int step_count_;
    int tie_countdown_;

    void step(OutputLogger &logger)
    {
        board_->update();

        TankAction action1 = algo1_->decideAction(*tank1_, *board_);
        TankAction action2 = algo2_->decideAction(*tank2_, *board_);

        bool valid1 = board_->executeTankAction(tank1_, action1);
        bool valid2 = board_->executeTankAction(tank2_, action2);

        logger.logAction(1, step_count_, action1, valid1);
        logger.logAction(2, step_count_, action2, valid2);

        ++step_count_;
        // TODO: add countdown for tie and and another general one to avoid infinite loop
        if (tie_countdown_ > 0)
            --tie_countdown_;
    }

    bool isGameOver() const
    {
        if (!tank1_->isAlive() && !tank2_->isAlive())
            return true;
        if (!tank1_->isAlive() || !tank2_->isAlive())
            return true;
        if (tie_countdown_ == 0)
            return true;
        return false;
    }
};
