#include "algorithms/smart_algorithm.h"
#include "algorithms/algorithm_utils.h"
#include <iostream>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <optional>


// Checks whether there is a dangerous shell approaching the given position
bool SmartAlgorithm::isShellInPathDangerous(const Position& pos, const Board& board) 
{
    for (int d = 0; d < 8; ++d) 
    {
        // Check all directions
        Direction dir = static_cast<Direction>(d);
        Position current = pos;
        for (int i = 0; i < 2; ++i) 
        {
            // Check 2 steps in each direction
            current = board.forward_position(current, dir);
            const Cell& cell = board.getCell(current);
            // Check for a shell moving towards the current position
            if (cell.has(ObjectType::Shell) && 
                static_cast<Shell*>(cell.get_object(ObjectType::Shell).get())->direction() == getOppositeDirection(dir)) 
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

        if (iterations > 5000) 
        {
            std::cout << "[SmartAlgorithm] BFS aborted after too many iterations!" << std::endl;
            break;
        }

        BFSState current = q.front(); 

        if (VERBOSE_DEBUG)
        {
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
            while (parent.find(current) != parent.end() && parent[current].first != startState) 
            {
                current = parent[current].first;
                std::cout << tankActionToString(parent[current].second) << " -> " << std::endl;
            }
            std::cout << "[SmartAlgorithm] First move to execute: "
            << tankActionToString(parent[current].second) << std::endl;
            return parent[current].second;
        }

        // Try moving forward if safe
        Position nextPos = board.forward_position(current.pos, current.dir);
        const Cell& nextCell = board.getCell(nextPos);
        if (nextCell.empty() && !isShellInPathDangerous(nextPos, board)) 
        {
            BFSState nextState{nextPos, current.dir};

            if (VERBOSE_DEBUG)
            {
                // For debugging purposes
                std::cout << "[SmartAlgorithm] Considering move forward to Pos(" 
                << nextPos.first << "," << nextPos.second 
                << ") Dir=" << directionToString(current.dir) 
                << ") -> visited? " << (visited.count(nextState) ? "yes" : "no") << std::endl;
            }

            if (visited.find(nextState) == visited.end()) 
            {
                if (VERBOSE_DEBUG)
                {
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

            if (VERBOSE_DEBUG)
            {
                // For debugging purposes
                std::cout << "[SmartAlgorithm] Considering rotation to Dir=" 
                << directionToString(newDir) << " from Pos(" 
                << current.pos.first << "," << current.pos.second 
                << ") -> visited? " << (visited.count(rotatedState) ? "yes" : "no") << std::endl;
            }


            if (visited.find(rotatedState) == visited.end()) 
            {
                if (VERBOSE_DEBUG)
                {
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
        return *evade;
    }

    // If not in danger, check if we can shoot the opponent
    const std::shared_ptr<Tank> opponent = board.get_player_tank(tank.id() == 1 ? 2 : 1);
    if (opponent && opponent->is_alive() && hasLineOfSight(tank.position(), opponent->position(), tank.direction(), board)) 
    {

        std::cout << "[SmartAlgorithm] Shooting opponent at (" << opponent->position().first << "," << opponent->position().second << ")" <<
         " from (" << tank.position().first << "," << tank.position().second << ")" << std::endl;

        return TankAction::Shoot;
    }

    // If we can't shoot, try to find a safe path towards the opponent
    if (opponent) 
    {
        if (auto move = findFirstSafeActionToOpponent(board, tank.position(), tank.direction(), opponent->position())) 
        {
            return *move;
        }
    }

    // If no action is found, remain idle
    return TankAction::Idle;
}