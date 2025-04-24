#pragma once

#include <fstream>

#include "types/tank_action.h"
#include "tank.h"

class OutputLogger
{
    std::ofstream out_;

public:
    OutputLogger(const std::string &filename) : out_(filename) {}
    void logAction(int player, int step, TankAction action, bool valid)
    {
        out_ << "Step " << step << " | Player " << player << " | Action: " << static_cast<int>(action)
             << (valid ? " OK" : " BAD") << "\n";
    }
    void logResult(const Tank &t1, const Tank &t2, int step)
    {
        if (!t1.is_alive() && !t2.is_alive())
            out_ << "Result: Tie - Both tanks destroyed at step " << step << "\n";
        else if (!t1.is_alive())
            out_ << "Result: Player 2 wins - Tank 1 destroyed\n";
        else if (!t2.is_alive())
            out_ << "Result: Player 1 wins - Tank 2 destroyed\n";
        else
            out_ << "Result: Tie - Time expired\n";
    }
};
