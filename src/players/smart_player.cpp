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
// TODO: Fix, walls in the way should stop shells and not be considered as possible directions.
//       Add all possible directions, not just the first that matches with the smallest number of turns passed.         
void SmartPlayer::updateShellPossibleDirections(const std::vector<std::vector<Cell>>& prev_grid,
                                                const std::vector<std::vector<Cell>>& curr_grid)
{
    // Save previous possible directions for narrowing down the possible directions
    auto prev_shell_possible_directions = shell_possible_directions_;
    shell_possible_directions_.clear();

    static const std::vector<Direction> all_directions = {
        Direction::U, Direction::UR, Direction::R, Direction::DR,
        Direction::D, Direction::DL, Direction::L, Direction::UL
    };

    size_t interval = config::get<size_t>("battle_info_interval");  // Max turns passed since last GetBattleInfo
    bool found_valid_turns = false;

    // For each shell, accumulate possible directions for all valid number of turns passed
    std::unordered_map<Position, std::unordered_set<Direction>> accumulated_directions;

    // Try to find valid number of turns passed that match all shells' movement
    for (size_t turns_passed = 0; turns_passed <= interval; ++turns_passed) 
    {
        std::unordered_map<Position, std::unordered_set<Direction>> candidate_map;
        bool all_shells_possible = true;
        
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
                    bool blocked = false;
                    // Make sure not to count directions blocked by walls
                    // We allow tanks be in the way, because they could have moved in after the shell 
                    // and we want this function to catch all possible directions
                    for (size_t step = 1; step <= 2 * turns_passed; ++step) 
                    {
                        Position intermediate = backward_position(curr_pos, dir, width_, height_, step);
                        if (prev_grid[intermediate.first][intermediate.second].has(ObjectType::Wall)) 
                        {
                            blocked = true;
                            break;
                        }
                    }
                    if (blocked) continue;

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
                            // We don't know this shell yet, every direction is possible
                            possible_dirs.insert(dir);
                        }
                    }
                }
                if (possible_dirs.empty()) 
                {
                    all_shells_possible = false;
                    break;
                }
                candidate_map[curr_pos] = std::move(possible_dirs);
            }
            if (!all_shells_possible) break;
        }

        if (all_shells_possible) 
        {
            // Found valid number of turns passed, accumulate possible directions for wach shell
            found_valid_turns = true;
            
            for (const auto& [pos, dirs] : candidate_map) 
            {
                accumulated_directions[pos].insert(dirs.begin(), dirs.end());
            }
        }
    }
    
    if (found_valid_turns)
    {
        shell_possible_directions_ = std::move(accumulated_directions);
    }
    else
    {
        // If no valid turns found, treat all shells as new with all directions possible
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
}