#include "algorithms/smart_algorithm.h"

#include <algorithm>
#include <iostream>
#include <optional>
#include <queue>
#include <unordered_map>
#include <unordered_set>

#include "TankAlgorithmRegistration.h"
#include "global_config.h"
#include "utils.h"

REGISTER_TANK_ALGORITHM(SmartAlgorithm)


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
            if constexpr (config::get<bool>("verbose_debug"))
            {
                std::cout << "[SmartAlgorithm] Cached path intersects with other tanks' reserved positions, "
                             "invalidating cached path."
                          << std::endl;
            }
            cached_path_ = {};
            break;
        }
    }

    // Invalidate cached path if the opponent moved
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

    // Invalidate cached path if any wall on the board has been damaged
    // Maybe it opened up more opportunities for us to find a better path
    auto previous_walls_damage = total_walls_damage_;

    // Update the player with our walls damage, save updated walls damage map, and clear the local damage map
    info.setWallsDamage(local_walls_damage_);
    total_walls_damage_ = info.getWallsDamage();
    local_walls_damage_.clear();

    // The walls damage map is incremental, which means if it's different from what we had before,
    // then there's a new wall that has been damaged
    if (total_walls_damage_ != previous_walls_damage)
    {
        if constexpr (config::get<bool>("verbose_debug"))
        {
            std::cout << "[SmartAlgorithm] Walls damage changed, invalidating cached path." << std::endl;
        }

        cached_path_ = {}; // Invalidate cached path
    }

    // Update the player with our reserved positions, doing last because path might be invalidated
    info.setTankReservedPositions(tank_index_, computeReservedPositions(true));
}

std::unordered_set<Position> SmartAlgorithm::computeReservedPositions(bool include_shooting_lane)
{
    if (cached_path_.empty())
        return {}; // No cached path, nothing to reserve

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
            [[fallthrough]];
        case ActionRequest::RotateRight90:
            [[fallthrough]];
        case ActionRequest::RotateLeft45:
            [[fallthrough]];
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

    // Print walls damage
    std::cout << "[SmartAlgorithm] Player " << player_index_
              << " Tank " << tank_index_
              << " total walls damage: ";

    size_t wall_count = 0;
    size_t total_walls = total_walls_damage_.size();

    for (const auto& [pos, damage] : total_walls_damage_)
    {
        std::cout << pos << " -> " << damage;
        if (++wall_count < total_walls)
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

void SmartAlgorithm::extendShootActionHandling()
{
    // If we shoot a wall, we need to update the walls damage map
    Position next_pos = forwardPosition(tank_->position(), tank_->direction(), width_, height_);
    const Cell& next_cell = grid_[next_pos.first][next_pos.second];

    if (next_cell.has(ObjectType::Wall))
    {
        local_walls_damage_[next_pos]++; // Increment the wall damage count
        total_walls_damage_[next_pos]++; // For easier planning
    }
}

bool SmartAlgorithm::isCellEmptyInState(const BFSState& state, const Position& pos) const
{
    const Cell& cell = grid_[pos.first][pos.second];

    if (cell.empty())
    {
        return true; // Cell is empty
    }

    if (cell.has(ObjectType::Wall))
    {
        // Check if the wall has been damaged enough and destroyed
        size_t hits_before_search = getOrDefault(total_walls_damage_, pos, (size_t)0);
        size_t hits_in_state = getOrDefault(state.walls_damage, pos, (size_t)0);

        return (hits_before_search + hits_in_state) >= 2; // Wall is destroyed if it has been hitten twice
    }

    return false; // Cell is not empty
}

void SmartAlgorithm::tryRotations(std::queue<BFSState>& q,
                                  std::unordered_map<BFSState, std::pair<BFSState, ActionRequest>>& parent,
                                  std::unordered_set<BFSState>& visited,
                                  const BFSState& current)
{
    static constexpr std::array<ActionRequest, 4> rotations = {
        ActionRequest::RotateLeft90, ActionRequest::RotateLeft45,
        ActionRequest::RotateRight45, ActionRequest::RotateRight90};

    for (ActionRequest action : rotations)
    {
        Direction new_dir = getDirectionAfterRotation(current.dir, action);
        BFSState rotated_state = current;
        rotated_state.dir = new_dir;

        if (rotated_state.cooldown > 0)
            rotated_state.cooldown--;

        // Check if the next state has already been visited
        if (visited.find(rotated_state) == visited.end())
        {
            visited.insert(rotated_state);
            parent[rotated_state] = {current, action};
            q.push(rotated_state);
        }
    }
}

void SmartAlgorithm::tryForwardMove(std::queue<BFSState>& q,
                                    std::unordered_map<BFSState, std::pair<BFSState, ActionRequest>>& parent,
                                    std::unordered_set<BFSState>& visited,
                                    const BFSState& current)
{
    Position next_pos = forwardPosition(current.pos, current.dir, width_, height_);

    // Check if the next cell is empty, not threatened by a shell, and not reserved by another tank
    if (isCellEmptyInState(current, next_pos) && !isShellIncoming(next_pos) &&
        !other_tanks_reserved_positions_.count(next_pos))
    {
        BFSState next_state = current;
        next_state.pos = next_pos;

        if (next_state.cooldown > 0)
            next_state.cooldown--;

        // Check if the next state has already been visited
        if (visited.find(next_state) == visited.end())
        {
            visited.insert(next_state);
            parent[next_state] = {current, ActionRequest::MoveForward};
            q.push(next_state);
        }
    }
}

void SmartAlgorithm::tryGetBattleInfo(std::queue<BFSState>& q,
                                      std::unordered_map<BFSState, std::pair<BFSState, ActionRequest>>& parent,
                                      std::unordered_set<BFSState>& visited,
                                      const BFSState& current)
{
    BFSState next_state = current;

    if (next_state.cooldown > 0)
        next_state.cooldown--;

    // Check if the next state has already been visited
    if (visited.find(next_state) == visited.end())
    {
        visited.insert(next_state);
        parent[next_state] = {current, ActionRequest::GetBattleInfo};
        q.push(next_state);
    }
}

void SmartAlgorithm::tryShootingAWall(std::queue<BFSState>& q,
                                      std::unordered_map<BFSState, std::pair<BFSState, ActionRequest>>& parent,
                                      std::unordered_set<BFSState>& visited,
                                      const BFSState& current)
{
    if (current.shells_left <= 1 || current.cooldown > 0)
    {
        // Never shoot if we have only one shell left, as we need it to shoot the opponent.
        // Can't shoot if there's cooldown left.
        return;
    }

    Position next_pos = forwardPosition(current.pos, current.dir, width_, height_);
    const Cell& next_cell = grid_[next_pos.first][next_pos.second];

    // Check if the next cell is a wall and has not destroyed yet
    if (next_cell.has(ObjectType::Wall))
    {
        size_t hits_before_search = getOrDefault(total_walls_damage_, next_pos, (size_t)0);
        size_t hits_in_state = getOrDefault(current.walls_damage, next_pos, (size_t)0);

        if (hits_before_search + hits_in_state < 2)
        {
            // The wall has not been destroyed yet, we can try to shoot it
            BFSState next_state = current;
            next_state.shells_left--;
            next_state.cooldown = 4;
            next_state.walls_damage[next_pos] = hits_in_state + 1;

            // Check if the next state has already been visited
            if (visited.find(next_state) == visited.end())
            {
                visited.insert(next_state);
                parent[next_state] = {current, ActionRequest::Shoot};
                q.push(next_state);
            }
        }
    }
}

ActionRequest SmartAlgorithm::handleLineOfSightToOpponent(BFSState& current,
                                                          std::unordered_map<BFSState, std::pair<BFSState, ActionRequest>>& parent,
                                                          const BFSState& start_state,
                                                          const Position& opponent_pos)
{
    // Found line of sight to target, reconstruct the first move.
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

// Finds the shortest path to shoot the opponent using BFS, then breaks ties by choosing the path whose end is closest to the opponent.
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

    BFSState start_state{tank_->position(), tank_->direction(), tank_->ammo(), tank_->cooldown(), {}};
    q.push(start_state);
    visited.insert(start_state);

    bool found = false;
    std::vector<std::pair<BFSState, Position>> candidates; // (state, opponent position)

    size_t iterations = 0;
    size_t iterations_limit = config::get<size_t>("bfs_iterations_limit");

    while (!q.empty() && !found)
    {
        if (iterations > iterations_limit)
        {
            std::cout << "[SmartAlgorithm] BFS aborted after too many iterations!" << std::endl;
            break;
        }

        size_t layer_size = q.size(); // Number of states in the current layer

        // Process all states in the current layer
        for (size_t i = 0; i < layer_size; ++i)
        {
            BFSState current = q.front();
            q.pop();

            if constexpr (config::get<bool>("verbose_debug"))
            {
                // For debugging purposes
                if (++iterations % 5000 == 0)
                    std::cout << "Visited: " << visited.size() << ", Queue: " << q.size() << std::endl;
            }

            // If we have line of sight to the opponent, we found a shortest path, can add it to candidates
            Position opponent_pos;
            if (current.cooldown == 0 &&
                hasLineOfSightToOpponent(current.pos, current.dir, opponent_pos))
            {
                // We require cooldown to be 0, because we want shortest path to shoot the opponent
                found = true;
                candidates.emplace_back(current, opponent_pos); // Store the state and opponent position
                continue;                                       // Still process the rest of the layer
            }

            // Try GetBattleInfo for reducing cooldown
            tryGetBattleInfo(q, parent, visited, current);

            // Try moving forward if safe
            tryForwardMove(q, parent, visited, current);

            // Try rotating in all directions
            tryRotations(q, parent, visited, current);

            // Try shooting a wall
            tryShootingAWall(q, parent, visited, current);
        }

        // If we found a shortest path, we exit after finishing this layer
    }

    // Pick the best candidate from the found shortest paths
    if (!candidates.empty())
    {
        // Choose the candidate closest to the opponent
        auto best = std::min_element(candidates.begin(), candidates.end(),
                                     [this](const auto& a, const auto& b)
                                     {
                                         return getDistance(a.first.pos, a.second, a.first.dir, width_, height_) <
                                                getDistance(b.first.pos, b.second, b.first.dir, width_, height_);
                                     });

        return handleLineOfSightToOpponent(best->first, parent, start_state, best->second);
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
