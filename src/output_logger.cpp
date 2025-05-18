#include "output_logger.h"

OutputLogger::OutputLogger(const std::string& filename, const size_t total_tanks) : out_(filename), total_tanks_(total_tanks) {
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

bool OutputLogger::is_valid() const {
    return valid_;
}

void OutputLogger::logAction(size_t tank_no, std::optional<ActionRequest> action, bool valid, bool is_alive)
{
    if (!valid_)
    {
        return;
    }

    if (!action) {
        out_ << "killed";
        valid = true; // To skip the ignored message
        is_alive = true; // To skip the killed message
    } else {
        out_ << action_to_string(*action);
    }

    if (!valid)
    {
        out_ << " (ignored)";
    }

    if (!is_alive)
    {
        out_ << " (killed)";
    }

    if (tank_no < total_tanks_ - 1)
    {
        out_ << ", ";
    } else {
        out_ << std::endl;
    }
}

void OutputLogger::logResult(std::string&& result) {
    if (!valid_)
    {
        return;
    }

    out_ << std::move(result) << std::endl;
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
