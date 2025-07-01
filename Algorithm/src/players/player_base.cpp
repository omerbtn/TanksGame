#include "players/player_base.h"

#include "global_config.h"
#include "utils.h"


PlayerBase::PlayerBase(int player_index, size_t x, size_t y, size_t max_steps, size_t num_shells)
    : player_index_(player_index), width_(x), height_(y), max_steps_(max_steps), num_shells_(num_shells) {}

void PlayerBase::setShellsAsNew(const std::vector<std::vector<Cell>>& grid)
{
    const auto& all_directions = getAllDirections();
    for (size_t x = 0; x < width_; ++x)
    {
        for (size_t y = 0; y < height_; ++y)
        {
            Position curr_pos{x, y};
            if (!grid[x][y].has(ObjectType::Shell))
                continue;
            shell_possible_directions_[curr_pos] = std::unordered_set<Direction>(all_directions.begin(), all_directions.end());
        }
    }
}

// Derives all possible directions for shells based on the previous and current grid states.
void PlayerBase::updateShellPossibleDirections(const std::vector<std::vector<Cell>>& prev_grid,
                                               const std::vector<std::vector<Cell>>& curr_grid)
{
    // Save previous possible directions for narrowing down the possible directions
    auto prev_shell_possible_directions = shell_possible_directions_;
    shell_possible_directions_.clear();
    possible_turns_passed_.clear();

    size_t interval = config::get<size_t>("battle_info_interval"); // Maximum number of turns passed since the last GetBattleInfo request
    size_t num_shells = getNumberOfShellsInGrid(curr_grid);

    // Try to find possible directions for as many shells as possible, when the number of unexplainable shells is from 0 to num_shells
    for (size_t max_unexplainable = 0; max_unexplainable <= num_shells; ++max_unexplainable)
    {
        bool found_valid = false;

        // For each shell, accumulate possible directions for all valid number of turns passed
        std::unordered_map<Position, std::unordered_set<Direction>> accumulated_directions;

        // Try to find valid number of turns passed that match all shells' movements
        for (size_t turns_passed = 0; turns_passed <= interval; ++turns_passed)
        {
            std::unordered_map<Position, std::unordered_set<Direction>> candidate_map;
            std::vector<Position> unexplainable_shells;

            getShellPossibleDirectionsForTurnsPassed(
                prev_grid, curr_grid, prev_shell_possible_directions, turns_passed,
                candidate_map, unexplainable_shells);

            if (unexplainable_shells.size() <= max_unexplainable)
            {
                // If we have less unexplainable shells than allowed, we can consider this a valid number of turns passed
                // and accumulate possible directions for each shell
                found_valid = true;
                possible_turns_passed_.insert(turns_passed);
                accumulateDirections(accumulated_directions, candidate_map, unexplainable_shells);
            }
        }

        if (found_valid)
        {
            // Found valid possible directions with as few unexplainable shells as possible
            shell_possible_directions_ = std::move(accumulated_directions);
            return;
        }
    }

    // Fallback: treat all shells as new with all directions possible
    // Shouldn't happen, as this is the case of all shells are unexplainable
    setShellsAsNew(curr_grid);
}

void PlayerBase::accumulateDirections(std::unordered_map<Position, std::unordered_set<Direction>>& accumulated_directions,
                                      std::unordered_map<Position, std::unordered_set<Direction>>& candidate_map,
                                      const std::vector<Position>& unexplainable_shells)
{
    const auto& all_directions = getAllDirections();

    // For unexplainable shells, treat as new with all directions possible
    for (const auto& pos : unexplainable_shells)
    {
        candidate_map[pos] = std::unordered_set<Direction>(all_directions.begin(), all_directions.end());
    }

    // Accumulate possible directions for all shells
    for (const auto& [pos, dirs] : candidate_map)
    {
        accumulated_directions[pos].insert(dirs.begin(), dirs.end());
    }
}

void PlayerBase::getShellPossibleDirectionsForTurnsPassed(const std::vector<std::vector<Cell>>& prev_grid,
                                                          const std::vector<std::vector<Cell>>& curr_grid,
                                                          const std::unordered_map<Position, std::unordered_set<Direction>>& prev_shell_possible_directions,
                                                          size_t turns_passed,
                                                          std::unordered_map<Position, std::unordered_set<Direction>>& candidate_map,
                                                          std::vector<Position>& unexplainable_shells)
{
    const auto& all_directions = getAllDirections();

    for (size_t x = 0; x < width_; ++x)
    {
        for (size_t y = 0; y < height_; ++y)
        {
            Position curr_pos{x, y};

            if (!curr_grid[x][y].has(ObjectType::Shell))
                continue;

            // For every shell, find all possible directions it could have moved from
            std::unordered_set<Direction> possible_dirs;
            for (Direction dir : all_directions)
            {
                // Make sure not to count directions blocked by walls
                // We allow tanks and shells be in the way, because they could have moved in after the shell
                // and we want this function to catch all possible directions
                if (isBlockedByWall(prev_grid, curr_pos, dir, 2 * turns_passed))
                    continue;

                Position prev_pos = backwardPosition(curr_pos, dir, width_, height_, 2 * turns_passed);
                if (prev_grid[prev_pos.first][prev_pos.second].has(ObjectType::Shell))
                {
                    // If we have previous knowledge, intersect
                    auto it = prev_shell_possible_directions.find(prev_pos);
                    if (it != prev_shell_possible_directions.end())
                    {
                        if (it->second.count(dir))
                            possible_dirs.insert(dir);
                    }
                    else
                    {
                        // Shouldn't happen, because we should know all shells from prev_grid
                        possible_dirs.insert(dir);
                    }
                }
            }
            if (possible_dirs.empty())
            {
                unexplainable_shells.push_back(curr_pos);
            }
            candidate_map[curr_pos] = std::move(possible_dirs); // If is empty, will fill with all directions if allowed
        }
    }
}

SmartBattleInfo PlayerBase::createBattleInfo(const SatelliteView& satellite_view)
{
    Position tank_position;
    auto prev_grid = grid_;
    grid_ = reconstructGridFromSatelliteView(satellite_view, height_, width_, player_index_, num_shells_, tank_position);

    // Update shell directions before constructing info
    updateShellPossibleDirections(prev_grid, grid_);

    return SmartBattleInfo(satellite_view, height_, width_, max_steps_, num_shells_, shell_possible_directions_);
}