#include "output_logger.h"


OutputLogger::OutputLogger(const std::string& filename, const size_t total_tanks) : out_(filename), total_tanks_(total_tanks)
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

bool OutputLogger::is_valid() const
{
    return valid_;
}

void OutputLogger::logAction(size_t tank_no, std::optional<ActionRequest> action, bool valid, bool was_alive_at_start, bool died_this_round)
{
    if (!valid_)
    {
        return;
    }

    if (!was_alive_at_start)
    {
        // Tank was already dead before this round started
        out_ << "killed";
    }
    else
    {
        // Tank was alive at start, so show its action
        if (action)
        {
            out_ << action_to_string(*action);
        }
        else
        {
            out_ << "DoNothing"; // Fallback, though this shouldn't happen for alive tanks
        }

        // Add (ignored) if action was invalid
        if (!valid)
        {
            out_ << " (ignored)";
        }

        // Add (killed) if tank died during this round
        if (died_this_round)
        {
            out_ << " (killed)";
        }
    }

    if (tank_no < total_tanks_ - 1)
    {
        out_ << ", ";
    }
    else
    {
        out_ << std::endl;
    }
}

void OutputLogger::logResult(std::string&& result)
{
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
    case ActionRequest::MoveForward:
        return "MoveForward";
    case ActionRequest::MoveBackward:
        return "MoveBackward";
    case ActionRequest::RotateLeft90:
        return "RotateLeft90";
    case ActionRequest::RotateRight90:
        return "RotateRight90";
    case ActionRequest::RotateLeft45:
        return "RotateLeft45";
    case ActionRequest::RotateRight45:
        return "RotateRight45";
    case ActionRequest::Shoot:
        return "Shoot";
    case ActionRequest::GetBattleInfo:
        return "GetBattleInfo";
    case ActionRequest::DoNothing:
        return "DoNothing";
    default:
        return "Unknown Action";
    }
}
