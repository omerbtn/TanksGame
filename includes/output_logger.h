#pragma once

#include <fstream>

#include "types/tank_action.h"
#include "tank.h"

class OutputLogger
{
    std::ofstream out_;
    bool valid_ = false;

public:
    OutputLogger(const std::string& filename) {
        out_.open(filename);
        if (!out_.is_open()) {
            valid_ = false;
            std::cerr << "Warning: Failed to open log file: " << filename << std::endl;
        } else {
            valid_ = true;
        }
    }
    void logAction(int player, int step, TankAction action, bool valid)
    {
        if (!valid_) {
            return;
        }

        out_ << "Step " << step << " | Player " << player << " | Action: " << action_to_string(action) << " | "
             << (valid ? " OK" : " BAD") << std::endl;
    }
    void logResult(const Tank &t1, const Tank &t2, int step)
    {
        if (!valid_) {
            return;
        }

        if (!t1.is_alive() && !t2.is_alive())
            out_ << "Result: Tie - Both tanks destroyed at step " << step << std::endl;
        else if (!t1.is_alive())
            out_ << "Result: Player 2 wins - Tank 1 destroyed" << std::endl;
        else if (!t2.is_alive())
            out_ << "Result: Player 1 wins - Tank 2 destroyed" << std::endl;
        else
            out_ << "Result: Tie - Time expired" << std::endl;
    }

    std::string action_to_string(TankAction action) const
    {
        switch (action)
        {
        case TankAction::MoveForward:
            return "MoveForward";
        case TankAction::MoveBackward:
            return "MoveBackward";
        case TankAction::RotateLeft_1_8:
            return "RotateLeft_1_8";
        case TankAction::RotateRight_1_8:
            return "RotateRight_1_8";
        case TankAction::RotateLeft_1_4:
            return "RotateLeft_1_4";
        case TankAction::RotateRight_1_4:
            return "RotateRight_1_4";
        case TankAction::Shoot:
            return "Shoot";
        case TankAction::Idle:
            return "Idle";
        default:
            return "Unknown Action";
        }
    }
};
