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

// Currently unused, see 'AlgorithmBase::isShellIncoming'
// Checks whether there is a dangerous shell approaching the given position
bool SmartAlgorithm::isShellInPathDangerous(const Position& pos)
{
    for (int d = 0; d < 8; ++d)
    {
        // Check all directions
        Direction dir = static_cast<Direction>(d);
        Position current = pos;
        
        for (int i = 0; i < 4; ++i)
        {
            // Check 4 steps in each direction, as we might need time to evade
            current = forward_position(current, dir, width_, height_);
            const Cell& cell = grid_[current.first][current.second];

            // Check for a shell moving towards the current position
            if (cell.has(ObjectType::Shell) &&
                static_pointer_cast<Shell>(cell.get_object_by_type(ObjectType::Shell))->direction() == getOppositeDirection(dir))
            {
                return true;
            }
        }
    }
    return false;
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

        // if constexpr (config::get<bool>("verbose_debug"))
        // {
        //     // For debugging purposes
        //     std::cout << "[SmartAlgorithm] Visiting state: Pos" << current.pos
        //               << " Dir=" << directionToString(current.dir) << std::endl;
        // }

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
                            << tank_action_to_string(parent[current].second) << std::endl;
            }

            std::reverse(moves_reversed.begin(), moves_reversed.end());

            if constexpr (config::get<bool>("verbose_debug"))
            {
                std::cout << "[SmartAlgorithm] Path to opponent: ";
                for (size_t i = 0; i < moves_reversed.size(); ++i)
                {
                    std::cout << tank_action_to_string(moves_reversed[i]);
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
        Position next_pos = forward_position(current.pos, current.dir, width_, height_);
        const Cell& nextCell = grid_[next_pos.first][next_pos.second];

        // Check if the next cell is empty and not threatened by a shell
        if (nextCell.empty() && !isShellIncoming(next_pos))
        {
            BFSState next_state{next_pos, current.dir};

            // if constexpr (config::get<bool>("verbose_debug"))
            // {
            //     // For debugging purposes
            //     std::cout << "[SmartAlgorithm] Considering move forward to Pos" << next_pos
            //               << " Dir=" << directionToString(current.dir)
            //               << " -> visited? " << (visited.count(next_state) ? "yes" : "no") << std::endl;
            // }

            // Check if the next state has already been visited
            if (visited.find(next_state) == visited.end())
            {
                // if constexpr (config::get<bool>("verbose_debug"))
                // {
                //     // For debugging purposes
                //     std::cout << "[SmartAlgorithm] Pushing state: Pos" << next_state.pos
                //               << " Dir=" << directionToString(next_state.dir) << std::endl;
                // }

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

            // if constexpr (config::get<bool>("verbose_debug"))
            // {
            //     // For debugging purposes
            //     std::cout << "[SmartAlgorithm] Considering rotation to Dir="
            //               << directionToString(new_dir) << " from Pos" << current.pos
            //               << " -> visited? " << (visited.count(rotated_state) ? "yes" : "no") << std::endl;
            // }

            // Check if the next state has already been visited
            if (visited.find(rotated_state) == visited.end())
            {
                // if constexpr (config::get<bool>("verbose_debug"))
                // {
                //     // For debugging purposes
                //     std::cout << "[SmartAlgorithm] Pushing state: Pos" << rotated_state.pos
                //     << " Dir=" << directionToString(rotated_state.dir) << std::endl;
                // }

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
            std::cout << "[SmartAlgorithm] Evading a shell using: " << tank_action_to_string(*evade) << std::endl;
        }
        cached_path_ = {}; // We are moving, so invalidate path
        return *evade;
    }
    
    // Not under threat, check if we can shoot the opponent
    Position opponent_pos;
    if (hasLineOfSightToOpponent(tank_->position(), tank_->direction(), opponent_pos)) 
    {
        if (tank_->can_shoot())
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
            static_pointer_cast<Tank>(target_cell.get_object_by_type(ObjectType::Tank))->tank_id() == player_index_)
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
                std::cout << "[SmartAlgorithm] Following cached path, executing action: " << tank_action_to_string(cached_path_.front()) << std::endl;
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
                std::cout << "[SmartAlgorithm] Computed path using BFS, executing action: " << tank_action_to_string(*move) << std::endl;
            }
    
            cached_path_.pop(); // Remove the first action from the path (== move)
            return *move;
        }
    }

    // Always prefer requesting BattleInfo if we don't have something better to do
    return ActionRequest::GetBattleInfo;
}
