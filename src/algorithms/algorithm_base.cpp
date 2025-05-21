#include "algorithms/algorithm_base.h"

#include <cassert>

#include "board_battle_info.h"
#include "algorithms/algorithm_utils.h"
#include "global_config.h"


AlgorithmBase::AlgorithmBase(int player_index, int tank_index)
    : player_index_(player_index), tank_index_(tank_index) {}


bool AlgorithmBase::hasLineOfSightToOpponent(const Position& start, Direction dir, Position& r_opponent_pos) const
{
    Position current = forward_position(start, dir, width_, height_);

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
            auto tank = std::static_pointer_cast<Tank>(cell.get_object_by_type(ObjectType::Tank));
            if (tank->player_id() != player_index_) 
            {
                r_opponent_pos = current;
                return true; // Found an opponent
            }
            else
            {
                return false; // Don't shoot our own tanks
            }
        }

        current = forward_position(current, dir, width_, height_);
    }

    return false; // No opponent found
}

void AlgorithmBase::updateBattleInfo(BattleInfo& info) 
{
    auto& concrete_info = static_cast<BoardBattleInfo&>(info);
    grid_ = std::move(concrete_info.get_grid());
    width_ = grid_[0].size();
    height_ = grid_.size();
    
    if (!tank_created_) 
    {
        tank_ = std::make_shared<Tank>(std::move(concrete_info.get_tank()));
        tank_created_ = true;
    }
}

void AlgorithmBase::handle_tank_movement(const ActionRequest action) 
{
    tank_->decrease_cooldown();

    // TODO: Handle MoveBackward
    switch (action) 
    {
        case ActionRequest::MoveForward: 
        {
            Position current_pos = tank_->position();
            Position new_pos = forward_position(current_pos, tank_->direction(), width_, height_);

            Cell& current_cell = grid_[current_pos.first][current_pos.second];
            Cell& new_cell = grid_[new_pos.first][new_pos.second];

            new_cell.add_object(tank_);
            current_cell.remove_object(tank_);
            tank_->position() = new_pos;
            break;
        }
        case ActionRequest::RotateLeft90:   [[fallthrough]];
        case ActionRequest::RotateRight90:  [[fallthrough]];
        case ActionRequest::RotateLeft45:   [[fallthrough]];
        case ActionRequest::RotateRight45: 
        {
            Direction new_dir = getDirectionAfterRotation(tank_->direction(), action);
            tank_->direction() = new_dir;
            break;
        }
        case ActionRequest::Shoot: 
        {
            if (tank_->can_shoot()) {
                tank_->shoot();
            }

            // We don't keep track of the shells inside the algorithm.
            // TODO: Handle them
            break;
        }
        case ActionRequest::DoNothing:
            [[fallthrough]];
        case ActionRequest::GetBattleInfo: {
            break;
        }
        default: {
            assert(false);
            break;
        }
    }
}

ActionRequest AlgorithmBase::getAction() 
{
    if (should_request_info_ == 0) 
    {
        // We request battle info every battle_info_interval turns
        should_request_info_ = config::get<int>("battle_info_interval");
        return ActionRequest::GetBattleInfo;
    }

    --should_request_info_;

    ActionRequest action = getActionImpl();
    handle_tank_movement(action);
    return action;
}
