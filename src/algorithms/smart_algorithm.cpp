#include "algorithms/smart_algorithm.h"

#include <iostream>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <algorithm>

#include "algorithms/algorithm_utils.h"
#include "global_config.h"

// Checks whether there is a dangerous shell approaching the given position
bool SmartAlgorithm::isShellInPathDangerous(const Position& pos, const Board& board)
{
    for (int d = 0; d < 8; ++d)
    {
        // Check all directions
        Direction dir = static_cast<Direction>(d);
        Position current = pos;
        for (int i = 0; i < 4; ++i)
        {
            // Check 4 steps in each direction, as we might need time to evade
            current = board.forward_position(current, dir);
            const Cell& cell = board.get_cell(current);
            // Check for a shell moving towards the current position
            if (cell.has(ObjectType::Shell) &&
                static_cast<Shell*>(cell.get_object_by_type(ObjectType::Shell).get())->direction() == getOppositeDirection(dir))
            {
                return true;
            }
        }
    }
    return false;
}

std::optional<TankAction> SmartAlgorithm::findFirstSafeActionToOpponent(const Board& board, const Position& startPos, Direction startDir, const Position& targetPos)
{
    // For debugging purposes
    std::cout << "[SmartAlgorithm] Starting BFS toward opponent" << std::endl;

    std::queue<BFSState> q;
    std::unordered_map<BFSState, std::pair<BFSState, TankAction>> parent;
    std::unordered_set<BFSState> visited;

    BFSState startState{startPos, startDir};
    q.push(startState);
    visited.insert(startState);

    std::vector<TankAction> rotations = {
        TankAction::RotateLeft_1_4, TankAction::RotateLeft_1_8,
        TankAction::RotateRight_1_8, TankAction::RotateRight_1_4
    };

    int iterations = 0;

    while (!q.empty())
    {
        // For debugging purposes
        if (++iterations % 10 == 0)
            std::cout << "Visited: " << visited.size() << ", Queue: " << q.size() << std::endl;

        if (iterations > 20000)
        {
            std::cout << "[SmartAlgorithm] BFS aborted after too many iterations!" << std::endl;
            break;
        }

        BFSState current = q.front();

        if constexpr (config::get<bool>("verbose_debug")) {
            // For debugging purposes
            std::cout << "[SmartAlgorithm] Visiting state: Pos("
            << current.pos.first << "," << current.pos.second
            << ") Dir=" << directionToString(current.dir) << std::endl;
        }

        q.pop();

        // If we have line of sight to target, reconstruct the first move
        // In the next move we will be able to shoot!
        if (hasLineOfSight(current.pos, targetPos, current.dir, board))
        {
            // For debugging purposes
            std::cout << "[SmartAlgorithm] Found line of sight from Pos("
            << current.pos.first << "," << current.pos.second
            << ") Dir=" << directionToString(current.dir)
            << " to target at (" << targetPos.first << "," << targetPos.second << ")" << std::endl;

            std::cout << "[SmartAlgorithm] Backtracking to find first move to execute:" << std::endl;

            std::vector<TankAction> moves_reversed;
            while (parent.find(current) != parent.end() && parent[current].first != startState)
            {
                moves_reversed.push_back(parent[current].second);
                current = parent[current].first;
            }
            moves_reversed.push_back(parent[current].second);
            std::cout << "[SmartAlgorithm] First move to execute: "
            << tank_action_to_string(parent[current].second) << std::endl;

            std::reverse(moves_reversed.begin(), moves_reversed.end());

            for (const TankAction& action : moves_reversed)
            {
                std::cout << tank_action_to_string(action) << " -> " << std::endl;
                cached_path_.push(action);
            }

            return parent[current].second;
        }

        // Try moving forward if safe
        Position nextPos = board.forward_position(current.pos, current.dir);
        const Cell& nextCell = board.get_cell(nextPos);
        if (nextCell.empty() && !isShellInPathDangerous(nextPos, board))
        {
            BFSState nextState{nextPos, current.dir};

            if constexpr (config::get<bool>("verbose_debug")) {
                // For debugging purposes
                std::cout << "[SmartAlgorithm] Considering move forward to Pos("
                << nextPos.first << "," << nextPos.second
                << ") Dir=" << directionToString(current.dir)
                << ") -> visited? " << (visited.count(nextState) ? "yes" : "no") << std::endl;
            }

            if (visited.find(nextState) == visited.end())
            {
                if constexpr (config::get<bool>("verbose_debug")) {
                    // For debugging purposes
                    std::cout << "[SmartAlgorithm] Pushing state: Pos("
                    << nextState.pos.first << "," << nextState.pos.second
                    << ") Dir=" << directionToString(nextState.dir) << std::endl;
                }

                visited.insert(nextState);
                parent[nextState] = {current, TankAction::MoveForward};
                q.push(nextState);
            }
        }

        // Try rotating in all directions
        for (TankAction action : rotations)
        {
            Direction newDir = getDirectionAfterRotation(current.dir, action);
            BFSState rotatedState{current.pos, newDir};

            if constexpr (config::get<bool>("verbose_debug")) {
                // For debugging purposes
                std::cout << "[SmartAlgorithm] Considering rotation to Dir="
                << directionToString(newDir) << " from Pos("
                << current.pos.first << "," << current.pos.second
                << ") -> visited? " << (visited.count(rotatedState) ? "yes" : "no") << std::endl;
            }

            if (visited.find(rotatedState) == visited.end())
            {
                if constexpr (config::get<bool>("verbose_debug")) {
                    // For debugging purposes
                    std::cout << "[SmartAlgorithm] Pushing state: Pos("
                    << rotatedState.pos.first << "," << rotatedState.pos.second
                    << ") Dir=" << directionToString(rotatedState.dir) << std::endl;
                }

                visited.insert(rotatedState);
                parent[rotatedState] = {current, action};
                q.push(rotatedState);
            }
        }
    }

    // For debugging purposes
    std::cout << "[SmartAlgorithm] BFS failed to find a path from (" << startPos.first << "," << startPos.second <<
     ") to (" << targetPos.first << "," << targetPos.second << ")" << std::endl;

    return std::nullopt; // No path found
}

// Get the next action for the tank
TankAction SmartAlgorithm::decideAction(const Tank& tank, const Board& board)
{
    // For debugging purposes
    std::cout << "[SmartAlgorithm] decideAction called for Tank " << tank.id() << std::endl;

    // First, check if there's an incoming shell we must evade
    if (auto evade = getEvadeActionIfShellIncoming(tank, board))
    {
        std::cout << "[SmartAlgorithm] Evading a shell using: " << tank_action_to_string(*evade) << std::endl;
        cached_path_ = {}; // We are moving, so invalidate path
        return *evade;
    }

    // If not in danger, check if we can shoot the opponent or move towards him
    const std::shared_ptr<Tank> opponent = board.get_player_tank(tank.id() == 1 ? 2 : 1);
    if (opponent && opponent->is_alive())
    {
        // If we have line of sight now, shoot him!
        if (hasLineOfSight(tank.position(), opponent->position(), tank.direction(), board))
        {
            std::cout << "[SmartAlgorithm] Shooting opponent at (" << opponent->position().first << "," << opponent->position().second << ")" <<
             " from (" << tank.position().first << "," << tank.position().second << ")" << std::endl;
    
            return TankAction::Shoot;
        }

        // If the oppnent moved, invalidate path
        if (opponent->position() != cached_target_)
        {
            std::cout << "[SmartAlgorithm] Opponent moved, invalidating cached path" << std::endl;
            cached_path_ = {};
        }

        // If we can't shoot, try to find a safe path towards the opponent
        // If we have a cached path, follow it
        if (!cached_path_.empty())
        {
            std::cout << "[SmartAlgorithm] Following cached path, executing action: " << tank_action_to_string(cached_path_.front()) << std::endl;
            TankAction next_action = cached_path_.front();
            cached_path_.pop();
            return next_action;
        }

        // If we don't have a cached path, recompute it
        if (auto move = findFirstSafeActionToOpponent(board, tank.position(), tank.direction(), opponent->position()))
        {
            std::cout << "[SmartAlgorithm] Recomputed path using BFS, executing action: " << tank_action_to_string(*move) << std::endl;
            cached_target_ = opponent->position(); // Update opponent position
            cached_path_.pop(); // Remove the first action from the path (== move)
            return *move;
        }
    }

    // If no action is found, remain idle
    return TankAction::Idle;
}
