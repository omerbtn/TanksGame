#include "algorithms/smart_algorithm.h"

#include <iostream>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <algorithm>
#include <cassert>

#include "algorithms/algorithm_utils.h"
#include "global_config.h"


SmartAlgorithm::SmartAlgorithm(int player_index, int tank_index) 
    : AlgorithmBase(player_index, tank_index) {}

void SmartAlgorithm::extendBattleInfoProcessing(SmartBattleInfo& info)
{
    // Merge all other tanks' reserved positions into one set
    other_tanks_reserved_positions_.clear();
    const auto& tanks_reserved_positions = info.getTanksReservedPositions();
    for (const auto& [tank_id, positions] : tanks_reserved_positions)
    {
        if (tank_id != tank_index_) // Skip our own tank
        {
            other_tanks_reserved_positions_.insert(positions.begin(), positions.end());
        }
    }

    // Invalidate cached path if it intersects with any reserved positions
    auto our_reserved_positions = computeReservedPositions(false);
    for (const auto& pos : our_reserved_positions)
    {
        if (other_tanks_reserved_positions_.count(pos))
        {
            // If our reserved positions intersects with other tanks' reserved positions, invalidate the cached path
            cached_path_ = {};
            break;
        }
    }
    
    // Update the player with our reserved positions
    info.setTankReservedPositions(tank_index_, computeReservedPositions(true));
}

std::unordered_set<Position> SmartAlgorithm::computeReservedPositions(bool include_shooting_lane)
{
    if (cached_path_.empty()) return {}; // No cached path, nothing to reserve

    std::unordered_set<Position> reserved_positions;
    Position pos = tank_->position();
    Direction dir = tank_->direction();
    std::queue<ActionRequest> path_copy = cached_path_;

    // Simulate the path to shooting position
    while (!path_copy.empty())
    {
        auto action = path_copy.front();
        path_copy.pop();
        switch (action)
        {
            case ActionRequest::MoveForward:
                pos = forwardPosition(pos, dir, width_, height_);
                reserved_positions.insert(pos);
                break;
            case ActionRequest::RotateLeft90:
            case ActionRequest::RotateRight90:
            case ActionRequest::RotateLeft45:
            case ActionRequest::RotateRight45:
                dir = getDirectionAfterRotation(dir, action);
                break;
            default:
                break; // Do nothing for other actions
            // We currently don't handle MoveBackward as algorithm doesn't use it
        }
    }

    if (include_shooting_lane)
    {
        // Add the shooting lane
        pos = forwardPosition(pos, dir, width_, height_);
        while (pos != cached_target_)
        {
            reserved_positions.insert(pos);
            pos = forwardPosition(pos, dir, width_, height_);
        }
    }

    return reserved_positions;
}

void SmartAlgorithm::extendPrintTankInfo() const
{
    // Print other tanks' reserved positions
    std::cout << "[SmartAlgorithm] Player " << player_index_ 
              << " Tank " << tank_index_ 
              << " other tanks' reserved positions: ";
    
    size_t pos_count = 0;
    size_t total_pos = other_tanks_reserved_positions_.size();
    
    for (const auto& pos : other_tanks_reserved_positions_)
    {
        std::cout << pos;
        if (++pos_count < total_pos)
            std::cout << ", ";
    }
    std::cout << std::endl;

    // Print cached path
    if (!cached_path_.empty())
    {
        std::cout << "[SmartAlgorithm] Player " << player_index_ 
                  << " Tank " << tank_index_ 
                  << " cached path: ";

        std::queue<ActionRequest> path_copy = cached_path_;
        size_t action_count = 0;
        size_t total_actions = path_copy.size();
        
        while (!path_copy.empty())
        {
            std::cout << tankActionToString(path_copy.front());
            path_copy.pop();
            if (++action_count < total_actions)
                std::cout << " -> ";
        }
        std::cout << std::endl;
    }
}

std::optional<ActionRequest> SmartAlgorithm::findFirstSafeActionToOpponent()
{
    if constexpr (config::get<bool>("verbose_debug"))
    {
        // For debugging purposes
        std::cout << "[SmartAlgorithm] Starting BFS toward opponent" << std::endl;
    }

    std::queue<BFSState> q;
    std::unordered_map<BFSState, std::pair<BFSState, ActionRequest>> parent;
    std::unordered_set<BFSState> visited;

    BFSState start_state{tank_->position(), tank_->direction()};
    q.push(start_state);
    visited.insert(start_state);

    static const std::vector<ActionRequest> rotations = {ActionRequest::RotateLeft90 , ActionRequest::RotateLeft45, 
                                                         ActionRequest::RotateRight45, ActionRequest::RotateRight90};

    int iterations = 0;

    while (!q.empty())
    {
        if constexpr (config::get<bool>("verbose_debug"))
        {
            // For debugging purposes
            if (++iterations % 10 == 0)
                std::cout << "Visited: " << visited.size() << ", Queue: " << q.size() << std::endl;
        }

        if (iterations > 20000)
        {
            std::cout << "[SmartAlgorithm] BFS aborted after too many iterations!" << std::endl;
            break;
        }

        BFSState current = q.front();
        q.pop();
        
        // If we have line of sight to target, reconstruct the first move.
        Position opponent_pos;
        if (hasLineOfSightToOpponent(current.pos, current.dir, opponent_pos))
        {
            if constexpr (config::get<bool>("verbose_debug"))
            {
                // For debugging purposes
                std::cout << "[SmartAlgorithm] Found line of sight from Pos" << current.pos
                          << " Dir=" << directionToString(current.dir)
                          << " to target at " << opponent_pos << std::endl;
                
                std::cout << "[SmartAlgorithm] Backtracking to find first move to execute:" << std::endl;
            }
            
            cached_target_ = opponent_pos;
            std::vector<ActionRequest> moves_reversed;

            while (parent.find(current) != parent.end() && parent[current].first != start_state)
            {
                moves_reversed.push_back(parent[current].second);
                current = parent[current].first;
            }

            moves_reversed.push_back(parent[current].second);

            if constexpr (config::get<bool>("verbose_debug"))
            {
                std::cout << "[SmartAlgorithm] First move to execute: "
                            << tankActionToString(parent[current].second) << std::endl;
            }

            std::reverse(moves_reversed.begin(), moves_reversed.end());

            if constexpr (config::get<bool>("verbose_debug"))
            {
                std::cout << "[SmartAlgorithm] Path to opponent: ";
                for (size_t i = 0; i < moves_reversed.size(); ++i)
                {
                    std::cout << tankActionToString(moves_reversed[i]);
                    if (i + 1 < moves_reversed.size())
                        std::cout << " -> ";
                }
                std::cout << std::endl;
            }

            // Store the found path in cached_path_
            cached_path_ = std::queue<ActionRequest>(std::deque<ActionRequest>(moves_reversed.begin(), moves_reversed.end()));

            return parent[current].second;
        }

        // Try moving forward if safe
        Position next_pos = forwardPosition(current.pos, current.dir, width_, height_);
        const Cell& nextCell = grid_[next_pos.first][next_pos.second];

        // Check if the next cell is empty, not threatened by a shell, and not reserved by another tank
        if (nextCell.empty() && !isShellIncoming(next_pos) &&
            !other_tanks_reserved_positions_.count(next_pos))
        {
            BFSState next_state{next_pos, current.dir};

            // Check if the next state has already been visited
            if (visited.find(next_state) == visited.end())
            {
                visited.insert(next_state);
                parent[next_state] = {current, ActionRequest::MoveForward};
                q.push(next_state);
            }
        }

        // Try rotating in all directions
        for (ActionRequest action : rotations) 
        {
            Direction new_dir = getDirectionAfterRotation(current.dir, action);
            BFSState rotated_state{current.pos, new_dir};

            // Check if the next state has already been visited
            if (visited.find(rotated_state) == visited.end())
            {
                visited.insert(rotated_state);
                parent[rotated_state] = {current, action};
                q.push(rotated_state);
            }
        }
    }

    if constexpr (config::get<bool>("verbose_debug"))
    {
        // For debugging purposes
        std::cout << "[SmartAlgorithm] BFS failed to find a path to opponent from " << tank_->position() << std::endl;
    }

    return std::nullopt; // No path found
}

ActionRequest SmartAlgorithm::getActionImpl() 
{
    // First, check if there's an incoming shell we must evade
    if (auto evade = getEvadeActionIfShellIncoming()) 
    {
        if constexpr (config::get<bool>("verbose_debug"))
        {
            std::cout << "[SmartAlgorithm] Evading a shell using: " << tankActionToString(*evade) << std::endl;
        }
        cached_path_ = {}; // We are moving, so invalidate path
        return *evade;
    }
    
    // Not under threat, check if we can shoot the opponent
    Position opponent_pos;
    if (hasLineOfSightToOpponent(tank_->position(), tank_->direction(), opponent_pos)) 
    {
        if (tank_->canShoot())
        {
            // The opponent is in front of us and we can shoot him
            if constexpr (config::get<bool>("verbose_debug"))
            {
                std::cout << "[SmartAlgorithm] Shooting opponent at " << opponent_pos
                          << " from " << tank_->position() << std::endl;
            }
            return ActionRequest::Shoot;
        }
    }
    else
    {
        // If we have line of sight but just can't shoot yet, better to stay in place and request BattleInfo

        // If the oppnent moved, invalidate path
        const Cell& target_cell = grid_[cached_target_.first][cached_target_.second];
        if (!target_cell.has(ObjectType::Tank) ||
            static_pointer_cast<Tank>(target_cell.getObjectByType(ObjectType::Tank))->playerId() == player_index_)
        {
            if constexpr (config::get<bool>("verbose_debug"))
            {
                std::cout << "[SmartAlgorithm] Opponent moved, invalidating cached path." << std::endl;
            }
            cached_path_ = {}; // Invalidate cached path   
        }
    
        // We can't shoot, try to find a safe path towards the opponent
        // If we have a cached path, follow it
        if (!cached_path_.empty())
        {
            if constexpr (config::get<bool>("verbose_debug"))
            {
                std::cout << "[SmartAlgorithm] Following cached path, executing action: " << tankActionToString(cached_path_.front()) << std::endl;
            }
    
            ActionRequest next_action = cached_path_.front();
            cached_path_.pop();
            return next_action;
        }
    
        // If we don't have a cached path, compute it
        if (auto move = findFirstSafeActionToOpponent()) 
        {
            if constexpr (config::get<bool>("verbose_debug"))
            {
                std::cout << "[SmartAlgorithm] Computed path using BFS, executing action: " << tankActionToString(*move) << std::endl;
            }
    
            cached_path_.pop(); // Remove the first action from the path (== move)
            return *move;
        }
    }

    // Always prefer requesting BattleInfo if we don't have something better to do
    return ActionRequest::GetBattleInfo;
}
