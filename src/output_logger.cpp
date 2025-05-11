#include "output_logger.h"


OutputLogger::OutputLogger(const std::string& filename) : out_(filename) 
{
    if (!out_) 
    {
        valid_ = false;
        if (!filename.empty())
            std::cerr << "Warning: Failed to open log file: " << filename << std::endl;
    } 
    else 
    {
        valid_ = true;
    }
}

void OutputLogger::logAction(int player, int step, ActionRequest action, bool valid) 
{
    if (!valid_) 
    {
        return;
    }

    out_ << "Step " << step << " | Player " << player << " | Action: " << action_to_string(action) << " | "
         << (valid ? " OK" : " BAD") << std::endl;
}

void OutputLogger::logResult(const Tank &t1, const Tank &t2, int step) 
{
    if (!valid_) 
    {
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

std::string OutputLogger::action_to_string(ActionRequest action) const 
{
    switch (action)
    {
        case ActionRequest::MoveForward: return "MoveForward";
        case ActionRequest::MoveBackward: return "MoveBackward";
        case ActionRequest::RotateLeft90: return "RotateLeft_1_4";
        case ActionRequest::RotateRight90: return "RotateRight_1_4";
        case ActionRequest::RotateLeft45: return "RotateLeft_1_8";
        case ActionRequest::RotateRight45: return "RotateRight_1_8";
        case ActionRequest::Shoot: return "Shoot";
        case ActionRequest::GetBattleInfo: return "GetBattleInfo";
        case ActionRequest::DoNothing: return "DoNothing";
        default: return "Unknown Action";
    }
}