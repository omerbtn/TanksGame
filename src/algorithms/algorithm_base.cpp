#include "algorithms/algorithm_base.h"

#include <cassert>

#include "board_battle_info.h"
#include "algorithms/algorithm_utils.h"
#include "global_config.h"

void AlgorithmBase::updateBattleInfo(BattleInfo& info) {
    auto& concrete_info = static_cast<BoardBattleInfo&>(info);
    grid_ = std::move(concrete_info.get_grid());
    width_ = grid_[0].size();
    height_ = grid_.size();
    if (!tank_created_) {
        tank_ = std::make_shared<Tank>(std::move(concrete_info.get_tank()));
        tank_created_ = true;
    }
}

void AlgorithmBase::handle_tank_movement(const ActionRequest action) {
    tank_->decrease_cooldown();

    switch (action) {
        case ActionRequest::MoveForward: {
            Position current_pos = tank_->position();
            Position new_pos = forward_position(current_pos, tank_->direction(), width_, height_);

            Cell& current_cell = grid_[current_pos.first][current_pos.second];
            Cell& new_cell = grid_[new_pos.first][new_pos.second];

            new_cell.add_object(tank_);
            current_cell.remove_object(tank_);
            tank_->position() = new_pos;
            break;
        }
        case ActionRequest::RotateLeft90: {
            tank_->direction() = static_cast<Direction>((static_cast<int>(tank_->direction()) + 6) % 8);
            break;
        }
        case ActionRequest::RotateRight90: {
            tank_->direction() = static_cast<Direction>((static_cast<int>(tank_->direction()) + 2) % 8);
            break;
        }
        case ActionRequest::RotateLeft45: {
            tank_->direction() = static_cast<Direction>((static_cast<int>(tank_->direction()) + 7) % 8);
            break;
        }
        case ActionRequest::RotateRight45: {
            tank_->direction() = static_cast<Direction>((static_cast<int>(tank_->direction()) + 1) % 8);
            break;
        }
        case ActionRequest::Shoot: {
            if (tank_->can_shoot()) {
                tank_->shoot();
            }

            // We don't keep track of the shells inside the algorithm.
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

ActionRequest AlgorithmBase::getAction() {
    if (should_request_info_ == 0) {
        // We request battle info every battle_info_interval turns
        should_request_info_ = config::get<int>("battle_info_interval");
        return ActionRequest::GetBattleInfo;
    }

    --should_request_info_;

    ActionRequest action = getActionImpl();
    handle_tank_movement(action);
    return action;
}
