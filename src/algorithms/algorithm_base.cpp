#include "algorithms/algorithm_base.h"

#include "algorithms/algorithm_utils.h"
#include "global_config.h"
#include "printers/ansi_printer.h"
#include "printers/default_printer.h"
#include "smart_battle_info.h"


AlgorithmBase::AlgorithmBase(int player_index, int tank_index) : player_index_(player_index), tank_index_(tank_index) {}

bool AlgorithmBase::hasLineOfSightToOpponent(const Position& start, Direction dir, Position& r_opponent_pos) const
{
    Position current = forwardPosition(start, dir, width_, height_);

    for (size_t steps = 0; steps < std::max(width_, height_); ++steps)
    {
        if (current == start)
        {
            return false; // We are back to the starting position
        }

        const Cell& cell = grid_[current.first][current.second];

        if (cell.has(ObjectType::Wall))
        {
            return false; // Wall in the way
        }
        else if (cell.has(ObjectType::Tank))
        {
            auto tank = std::static_pointer_cast<Tank>(cell.getObjectByType(ObjectType::Tank));
            if (tank->playerId() != player_index_)
            {
                r_opponent_pos = current;
                return true; // Found an opponent
            }
            else
            {
                return false; // Don't shoot our own tanks
            }
        }

        // We allow mines and shells in the way, as mine doesn't block line of sight and shell will
        // probably go away until our shell arrives. If not, shell is moving towards us and shooting
        // at it might be a good idea.
        current = forwardPosition(current, dir, width_, height_);
    }

    return false; // No opponent found
}

// Checks if a shell is incoming towards the given position within the specified maximum distance
// Outputs the position of the closest shell and its possible threatening direction if exists
bool AlgorithmBase::isShellIncoming(const Position& pos,
                                    Position* r_shell_pos,
                                    Direction* r_shell_possible_dir,
                                    size_t shell_max_distance) const
{
    std::vector<Direction> directions_to_check = {Direction::U, Direction::UR, Direction::R, Direction::DR,
                                                  Direction::D, Direction::DL, Direction::L, Direction::UL};

    // Check up to `shell_max_distance` steps
    for (size_t steps = 1; steps <= shell_max_distance; ++steps)
    {
        std::vector<Direction> next_to_check;
        // Check all directions
        for (const auto& dir : directions_to_check)
        {
            Position current = backwardPosition(pos, dir, width_, height_, steps);
            const Cell& cell = grid_[current.first][current.second];

            if (cell.has(ObjectType::Wall))
            {
                // We hit a wall, it will protect us, no shell from this direction
                continue;
            }

            // If we made it through, the shell has a line of sight to us
            if (cell.has(ObjectType::Shell))
            {
                auto possible_directions = shell_possible_directions_.find(current);
                if (possible_directions == shell_possible_directions_.end() || possible_directions->second.count(dir))
                {
                    // If we have possible directions for this shell, check if the current direction
                    // is one of them. If not (shouldn't happen), assume the shell is incoming
                    if (r_shell_pos)
                    {
                        *r_shell_pos = current; // Output the position of the incoming shell
                    }
                    if (r_shell_possible_dir)
                    {
                        *r_shell_possible_dir = dir; // Output the possible dangerous direction of the shell
                    }
                    return true;
                }
            }

            // Continue scanning in this direction
            next_to_check.push_back(dir);
        }

        // Update directions for the next distance layer
        directions_to_check = std::move(next_to_check);
    }

    return false; // No incoming shell found
}

std::optional<ActionRequest> AlgorithmBase::getEvadeActionIfShellIncoming(size_t shell_max_distance) const
{
    // Check if there's an incoming shell towards the tank's position
    Position shell_pos;
    Direction shell_possible_dir;
    if (isShellIncoming(tank_->position(), &shell_pos, &shell_possible_dir, shell_max_distance))
    {
        // Shell is (might be) coming towards us, need to run away
        Position tank_pos = tank_->position();
        Direction tank_dir = tank_->direction();

        if constexpr (config::get<bool>("verbose_debug"))
        {
            std::cout << "[AlgorithmBase] Shell incoming from " << shell_pos
                      << " with possible direction " << directionToString(shell_possible_dir)
                      << " towards tank at " << tank_pos << std::endl;
        }

        // We don't want to run towards the shell, or at the opposite direction (because the shell
        // is faster)
        if (tank_dir != shell_possible_dir && tank_dir != getOppositeDirection(shell_possible_dir))
        {
            // We can just move forward and evade the shell
            // Before, we need to check if the next cell is safe
            Position next_pos = forwardPosition(tank_pos, tank_dir, width_, height_);
            const Cell& next_cell = grid_[next_pos.first][next_pos.second];

            if (next_cell.empty() && !isShellIncoming(next_pos, nullptr, nullptr, shell_max_distance))
            {
                // Next cell is empty and not threatened by a shell, we can move forward
                if constexpr (config::get<bool>("verbose_debug"))
                {
                    std::cout << "[AlgorithmBase] Evading shell by moving forward from " << tank_pos << " to "
                              << next_pos << std::endl;
                }
                return ActionRequest::MoveForward;
            }
        }

        // Else, we need to rotate away from the shell to a safe direction
        static const std::vector<ActionRequest> rotation_options = {ActionRequest::RotateLeft90, ActionRequest::RotateRight90,
                                                                    ActionRequest::RotateLeft45, ActionRequest::RotateRight45};

        for (auto rotate_action : rotation_options)
        {
            Direction new_dir = getDirectionAfterRotation(tank_dir, rotate_action);
            // Avoid facing toward the shell or the opposite direction
            if (new_dir != shell_possible_dir && new_dir != getOppositeDirection(shell_possible_dir))
            {
                Position new_pos = forwardPosition(tank_pos, new_dir, width_, height_);
                const Cell& new_cell = grid_[new_pos.first][new_pos.second];

                // Check if the next position after rotation is safe
                // curr_pos -> rotation -> MoveForward -> new_pos
                if (new_cell.empty() && !isShellIncoming(new_pos, nullptr, nullptr, shell_max_distance))
                {
                    // Rotate away from the shell, so next turn we can move forward
                    if constexpr (config::get<bool>("verbose_debug"))
                    {
                        std::cout << "[AlgorithmBase] Evading shell by rotating " << tankActionToString(rotate_action)
                                  << " so next turn we can move forward from " << tank_pos << " to " << new_pos
                                  << std::endl;
                    }
                    return rotate_action;
                }
            }
        }

        // If we reach here, we couldn't find a safe action to evade the shell
        auto possible_directions = shell_possible_directions_.find(shell_pos);
        if (possible_directions != shell_possible_directions_.end() && possible_directions->second.size() > 1)
        {
            // If we are not sure about the shell's direction, request for battle info to find it
            if constexpr (config::get<bool>("verbose_debug"))
            {
                std::cout << "[AlgorithmBase] Didn't find a safe action to evade the shell, "
                             "requesting BattleInfo to get more information."
                          << std::endl;
            }
            return ActionRequest::GetBattleInfo;
        }
        else
        {
            // Rotate hard right to open up more options and try again next turn
            if constexpr (config::get<bool>("verbose_debug"))
            {
                std::cout << "[AlgorithmBase] No safe action found, rotating right 90 degrees to "
                             "open up more options."
                          << std::endl;
            }
            return ActionRequest::RotateRight90;
        }
    }

    // No incoming shell found, no need to evade
    return std::nullopt;
}

void AlgorithmBase::updateBattleInfo(BattleInfo& info)
{
    auto& concrete_info = static_cast<SmartBattleInfo&>(info);

    height_ = concrete_info.getHeight();
    width_ = concrete_info.getWidth();
    size_t num_shells = concrete_info.getNumShells();

    Position tank_pos;
    const SatelliteView& satellite_view = concrete_info.getSatelliteView();
    grid_ = reconstructGridFromSatelliteView(satellite_view, height_, width_, player_index_, num_shells, tank_pos);
    shell_possible_directions_ = concrete_info.getShellPossibleDirections();

    auto tank_obj = grid_[tank_pos.first][tank_pos.second].getObjectByType(ObjectType::Tank);
    auto new_tank = std::static_pointer_cast<Tank>(tank_obj);

    // If we already have a tank, transfer all runtime state to the new tank object
    if (tank_)
    {
        new_tank->copyRuntimeStateFrom(*tank_);
    }
    tank_ = new_tank;

    extendBattleInfoProcessing(concrete_info);
}

void AlgorithmBase::handleTankMovement(const ActionRequest action)
{
    tank_->decreaseCooldown();

    switch (action)
    {
    case ActionRequest::MoveForward:
    {
        Position current_pos = tank_->position();
        Position new_pos = forwardPosition(current_pos, tank_->direction(), width_, height_);

        Cell& current_cell = grid_[current_pos.first][current_pos.second];
        Cell& new_cell = grid_[new_pos.first][new_pos.second];

        new_cell.addObject(tank_);
        current_cell.removeObject(tank_);
        tank_->position() = new_pos;
        break;
    }
    case ActionRequest::RotateLeft90:
        [[fallthrough]];
    case ActionRequest::RotateRight90:
        [[fallthrough]];
    case ActionRequest::RotateLeft45:
        [[fallthrough]];
    case ActionRequest::RotateRight45:
    {
        Direction new_dir = getDirectionAfterRotation(tank_->direction(), action);
        tank_->direction() = new_dir;
        break;
    }
    case ActionRequest::Shoot:
    {
        if (tank_->canShoot())
        {
            tank_->shoot();
            extendShootActionHandling();
        }

        // We don't keep track of the shells inside the algorithm. Too complicated and error-prone.
        // Player will keep track on shells and their directions.
        break;
    }
    case ActionRequest::DoNothing:
        [[fallthrough]];
    case ActionRequest::GetBattleInfo:
        break;
    case ActionRequest::MoveBackward:
        [[fallthrough]];
    default:
        break;
    }
}

void AlgorithmBase::printTankInfo() const
{
    // Print grid
    std::cout << "[AlgorithmBase] Player " << player_index_ << " Tank " << tank_index_ << " known grid:" << std::endl;
    printGrid(grid_);

    // Print tank's position
    if (tank_)
    {
        std::cout << "[AlgorithmBase] Player " << player_index_ << " Tank " << tank_index_
                  << " position: " << tank_->position() << std::endl;
    }

    // Print shells possible directions
    std::cout << "[AlgorithmBase] Player " << player_index_ << " Tank " << tank_index_
              << " known shell directions:" << std::endl;
    for (const auto& [pos, directions] : shell_possible_directions_)
    {
        std::cout << "Shell at " << pos << ": ";
        for (const auto& dir : directions)
        {
            std::cout << directionToString(dir) << " ";
        }
        std::cout << std::endl;
    }

    // Print additional information
    extendPrintTankInfo();
}

ActionRequest AlgorithmBase::getAction()
{
    if constexpr (config::get<bool>("verbose_debug"))
    {
        printTankInfo();
    }

    if (turns_till_next_battle_info_ == 0)
    {
        // We want no more than battle_info_interval turns between GetBattleInfo requests
        turns_till_next_battle_info_ = config::get<size_t>("battle_info_interval") - 1;
        if constexpr (config::get<bool>("verbose_debug"))
        {
            std::cout << "[AlgorithmBase] Too much time since last GetBattleInfo, requesting new "
                         "BattleInfo."
                      << std::endl;
        }
        return ActionRequest::GetBattleInfo;
    }

    --turns_till_next_battle_info_;

    ActionRequest action = getActionImpl();

    if (action == ActionRequest::GetBattleInfo)
    {
        turns_till_next_battle_info_ = config::get<size_t>("battle_info_interval") - 1; // Reset the counter
    }

    handleTankMovement(action);
    return action;
}
