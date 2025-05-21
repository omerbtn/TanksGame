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
            int x = current.first % width_;
            int y = current.second % height_;

            const Cell& cell = grid_[x][y];

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

std::optional<ActionRequest> SmartAlgorithm::findFirstSafeActionToOpponent(const Position& start_pos, Direction start_dir) 
{
    if constexpr (config::get<bool>("verbose_debug"))
    {
        // For debugging purposes
        std::cout << "[SmartAlgorithm] Starting BFS toward opponent" << std::endl;
    }

    std::queue<BFSState> q;
    std::unordered_map<BFSState, std::pair<BFSState, ActionRequest>> parent;
    std::unordered_set<BFSState> visited;

    BFSState start_state{start_pos, start_dir};
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

        if constexpr (config::get<bool>("verbose_debug"))
        {
            // For debugging purposes
            std::cout << "[SmartAlgorithm] Visiting state: Pos" << current.pos
                      << " Dir=" << directionToString(current.dir) << std::endl;
        }

        q.pop();
        
        // If we have line of sight to target, reconstruct the first move.
        // In the next move we will be able to shoot!
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

            for (const ActionRequest& action : moves_reversed) {
                if constexpr (config::get<bool>("verbose_debug"))
                {
                    std::cout << tank_action_to_string(action) << " -> " << std::endl;
                }
                cached_path_.push(action);
            }

            return parent[current].second;
        }

        // Try moving forward if safe
        Position next_pos = forward_position(current.pos, current.dir, width_, height_);
        const Cell& nextCell = grid_[next_pos.first][next_pos.second];

        // TODO: Think what to do about the shells, we don't always know their direction
        if (nextCell.empty() && !isShellInPathDangerous(next_pos))
        {
            BFSState next_state{next_pos, current.dir};

            if constexpr (config::get<bool>("verbose_debug"))
            {
                // For debugging purposes
                std::cout << "[SmartAlgorithm] Considering move forward to Pos" << next_pos
                          << " Dir=" << directionToString(current.dir)
                          << " -> visited? " << (visited.count(next_state) ? "yes" : "no") << std::endl;
            }

            // Check if the next state has already been visited
            if (visited.find(next_state) == visited.end())
            {
                if constexpr (config::get<bool>("verbose_debug"))
                {
                    // For debugging purposes
                    std::cout << "[SmartAlgorithm] Pushing state: Pos" << next_state.pos
                              << " Dir=" << directionToString(next_state.dir) << std::endl;
                }

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

            if constexpr (config::get<bool>("verbose_debug"))
            {
                // For debugging purposes
                std::cout << "[SmartAlgorithm] Considering rotation to Dir="
                          << directionToString(new_dir) << " from Pos" << current.pos
                          << " -> visited? " << (visited.count(rotated_state) ? "yes" : "no") << std::endl;
            }

            // Check if the next state has already been visited
            if (visited.find(rotated_state) == visited.end())
            {
                if constexpr (config::get<bool>("verbose_debug"))
                {
                    // For debugging purposes
                    std::cout << "[SmartAlgorithm] Pushing state: Pos" << rotated_state.pos
                    << " Dir=" << directionToString(rotated_state.dir) << std::endl;
                }

                visited.insert(rotated_state);
                parent[rotated_state] = {current, action};
                q.push(rotated_state);
            }
        }
    }

    if constexpr (config::get<bool>("verbose_debug"))
    {
        // For debugging purposes
        // std::cout << "[SmartAlgorithm] BFS failed to find a path from (" << start_pos.first << "," << start_pos.second <<
        //  ") to (" << target_pos.first << "," << target_pos.second << ")" << std::endl;
    }

    return std::nullopt; // No path found
}

// Get the next action for the tank
ActionRequest SmartAlgorithm::getActionImpl() 
{
    // First, check if there's an incoming shell we must evade
    /*if (auto evade = getEvadeActionIfShellIncoming(tank, board))
    {
        if constexpr (config::get<bool>("verbose_debug"))
        {
            std::cout << "[SmartAlgorithm] Evading a shell using: " << tank_action_to_string(*evade) << std::endl;
        }
        cached_path_ = {}; // We are moving, so invalidate path
        return *evade;
    }*/

    
    // If we have line of sight now, shoot him!
    Position opponent_pos;
    if (hasLineOfSightToOpponent(tank_->position(), tank_->direction(), opponent_pos)) 
    {
        if constexpr (config::get<bool>("verbose_debug"))
        {
            std::cout << "[SmartAlgorithm] Shooting opponent at " << opponent_pos
                      << " from " << tank_->position() << std::endl;
        }
        return ActionRequest::Shoot;
    }

    // If the oppnent moved, invalidate path
    // if (opponent->position() != cached_target_)
    // {
    //     if constexpr (config::get<bool>("verbose_debug"))
    //     {
    //         std::cout << "[SmartAlgorithm] Opponent moved, invalidating cached path" << std::endl;
    //     }
    //     cached_path_ = {};
    // }

    // If we can't shoot, try to find a safe path towards the opponent
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

    // If we don't have a cached path, recompute it
    if (auto move = findFirstSafeActionToOpponent(tank_->position(), tank_->direction())) 
    {
        if constexpr (config::get<bool>("verbose_debug"))
        {
            std::cout << "[SmartAlgorithm] Recomputed path using BFS, executing action: " << tank_action_to_string(*move) << std::endl;
        }

        cached_path_.pop(); // Remove the first action from the path (== move)
        return *move;
    }


    // If no action is found, do nothing
    return ActionRequest::DoNothing;
}
