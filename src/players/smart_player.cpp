#include "smart_player.h"
#include "algorithm_utils.h"
#include "smart_battle_info.h"
#include "global_config.h"


SmartPlayer::SmartPlayer(int player_index, size_t x, size_t y, size_t max_steps, size_t num_shells)
    : Player(player_index, x, y, max_steps, num_shells), 
      player_index_(player_index), width_(x), height_(y), max_steps_(max_steps), num_shells_(num_shells) {}

std::vector<std::vector<Cell>> SmartPlayer::reconstructGridFromSatelliteView(const SatelliteView& satellite_view, Position& r_tank_pos)
{
    std::vector<std::vector<Cell>> grid(height_, std::vector<Cell>(width_));

    for (size_t y = 0; y < height_; ++y) 
    {
        for (size_t x = 0; x < width_; ++x) 
        {
            char ch = satellite_view.getObjectAt(x, y);
            Position pos{x, y};
            Cell cell(pos);

            switch (ch) 
            {
                case '#':
                    cell.add_object(std::make_shared<Wall>());
                    break;
                case '@':
                    cell.add_object(std::make_shared<Mine>());
                    break;
                case '*':
                    cell.add_object(std::make_shared<Shell>(Direction::U));  // Direction is unreliable
                    break;
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    cell.add_object(std::make_shared<Tank>(ch - '0', 0, pos, getSeedDirection(ch - '0'), num_shells_));  // Direction is unreliable unless it's first turn
                    break;
                case '%':
                    cell.add_object(std::make_shared<Tank>(player_index_, 0, pos, getSeedDirection(player_index_), num_shells_));  // Direction is unreliable unless it's first turn
                    r_tank_pos = pos;
                    break;
                default:
                    break;
            }

            grid[x][y] = std::move(cell);
        }
    }

    return grid;
}

void SmartPlayer::updateTankWithBattleInfo(TankAlgorithm& tank, SatelliteView& satellite_view)
{
    Position tank_position;
    auto prev_grid = grid_;
    grid_ = std::move(reconstructGridFromSatelliteView(satellite_view, tank_position));

    // Update shell directions before constructing info
    updateShellPossibleDirections(prev_grid, grid_);

    SmartBattleInfo info(grid_, tank_position, max_steps_, num_shells_, shell_possible_directions_);
    tank.updateBattleInfo(info);
}

// Derives all possible directions for shells based on the previous and current grid states.
void SmartPlayer::updateShellPossibleDirections(const std::vector<std::vector<Cell>>& prev_grid,
                                                const std::vector<std::vector<Cell>>& curr_grid)
{
    // Save previous possible directions for narrowing down the possible directions
    auto prev_shell_possible_directions = shell_possible_directions_;
    shell_possible_directions_.clear();

    size_t interval = config::get<size_t>("battle_info_interval");  // Max turns passed since last GetBattleInfo
    size_t num_shells = getNumberOfShellsInGrid(curr_grid);
    
    // Try to find possible directions for as many shells as possible, when the number of unexplainable shells is from 0 to num_shells
    for (size_t max_unexplainable = 0; max_unexplainable < num_shells; ++max_unexplainable)
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
    const auto& all_directions = getAllDirections();
    for (size_t x = 0; x < width_; ++x) 
    {
        for (size_t y = 0; y < height_; ++y) 
        {
            Position curr_pos{x, y};
            if (!curr_grid[x][y].has(ObjectType::Shell)) continue;
            shell_possible_directions_[curr_pos] = std::unordered_set<Direction>(all_directions.begin(), all_directions.end());
        }
    }
}

void SmartPlayer::accumulateDirections(std::unordered_map<Position, std::unordered_set<Direction>>& accumulated_directions,
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

void SmartPlayer::getShellPossibleDirectionsForTurnsPassed(const std::vector<std::vector<Cell>>& prev_grid,
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
                if (isBlockedByWall(prev_grid, curr_pos, dir, 2 * turns_passed)) continue;

                Position prev_pos = backward_position(curr_pos, dir, width_, height_, 2 * turns_passed);
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
            candidate_map[curr_pos] = std::move(possible_dirs);  // If is empty, will fill with all directions if allowed
        }
    }
}