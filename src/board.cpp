#include "board.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <map>
#include <algorithm>

#include "tank.h"
#include "wall.h"
#include "mine.h"
#include "shell.h"
#include "player.h"

#include "algorithms/simple_algorithm.h"
#include "algorithms/smart_algorithm.h"
#include "algorithms/seed_algorithm.h"
#include "algorithms/algorithm_utils.h"
#include "input_errors_logger.h"
#include "global_config.h"


std::map<size_t, Player>& Board::players() 
{
    return players_;
}

std::vector<std::vector<Cell>>& Board::grid() 
{
    return grid_;
}

const std::string& Board::input_file_name() const 
{
    return input_file_name_;
}

bool Board::load_from_file(const std::string& filename) 
{
    input_file_name_ = filename;

    InputErrorLogger error_logger;
    std::ifstream file(filename);
    
    if (!file) 
    {
        error_logger.log("Couldn't open file: ", filename);
        return false;
    }

    file >> width_ >> height_;
    file.ignore();  // skip newline

    grid_.resize(height_, std::vector<Cell>(width_));

    std::string line;
    for (size_t y = 0; y < height_; ++y) 
    {
        std::getline(file, line);
        if (line.size() != width_) 
        {
            error_logger.log("Warning: The width of the ", y, " row is wrong. Filling the missing cells and ignoring the extra cells.");
        }

        for (size_t x = 0; x < width_ && x < line.size(); ++x) 
        {
            char ch = line[x];
            Position pos(x, y);
            switch (ch) 
            {
                case '#': 
                {
                    grid_[x][y] = Cell(pos, std::make_shared<Wall>());
                    break;
                }
                case '@': 
                {
                    grid_[x][y] = Cell(pos, std::make_shared<Mine>());
                    break;
                }
                case '1': 
                {
                    if (players_.contains(1)) 
                    {
                        error_logger.log("Warning: player 1 was already taken. Ignoring character '1' at (", x, ",", y,
                                         "). Treating as empty.");
                        break;
                    }
                    auto algo = std::make_shared<SmartAlgorithm>();
                    auto tank = std::make_shared<Tank>(1, pos, Direction::L);
                    auto player = Player(tank, algo);
                    players_[1] = player;
                    grid_[x][y] = Cell(pos, tank);
                    break;
                }
                case '2': 
                {
                    if (players_.contains(2)) 
                    {
                        error_logger.log("Warning: player 2 was already taken. Ignoring character '2' at (", x, ",", y,
                                         "). Treating as empty.");
                        break;
                    }
                    auto algo = std::make_shared<SimpleAlgorithm>();
                    auto tank = std::make_shared<Tank>(2, pos, Direction::R);
                    auto player = Player(tank, algo);
                    players_[2] = player;
                    grid_[x][y] = Cell(pos, tank);
                    break;
                }
                case '.': 
                case ' ':
                {
                    grid_[x][y] = Cell(pos);
                    break;
                }
                default: 
                {
                    // Unexpected character
                    error_logger.log("Warning: invalid character '", ch, "' at (", x, ",", y, "). Treating as empty.");
                    grid_[x][y] = Cell(pos);
                    break;
                }
            }
        }
    }

    if (players_.size() != config::get<size_t>("num_players")) 
    {
        error_logger.log("Warning: Not enough players. Aborting.");
        return false;
    }

    return true;
}

void Board::print() const 
{
    std::cout << "Game Board:" << std::endl;

    for (size_t y = 0; y < height_; ++y) 
    {
        for (size_t x = 0; x < width_; ++x) 
        {
            const Cell& cell = grid_[x][y];
            std::string to_print = "[";

            // Walls
            if (cell.has(ObjectType::Wall))
                to_print += "#";

            // Mines
            if (cell.has(ObjectType::Mine))
                to_print += "@";

            // Tanks
            if (cell.has(ObjectType::Tank)) 
            {
                const auto& tanks = cell.get_objects_by_type(ObjectType::Tank);
                if (!tanks.empty())
                {
                    auto tank = std::static_pointer_cast<Tank>(tanks.front()); // Printing just one tank, couldn't be more
                    to_print += (tank->id() == 1 ? '1' : '2');
                    to_print += directionToArrow(tank->direction());
                }
            }

            // Shells
            if (cell.has(ObjectType::Shell)) 
            {
                const auto& shells = cell.get_objects_by_type(ObjectType::Shell);
                if (!shells.empty())
                {
                    auto shell = std::static_pointer_cast<Shell>(shells.front()); // Printing just one shell, couldn't be more
                    to_print += "*";
                    to_print += directionToArrow(shell->direction());
                }
            }

            while (to_print.size() < 3)
                to_print += " ";
            
            to_print += "]";
            std::cout << to_print;
        }
        std::cout << std::endl;
    }
}

std::shared_ptr<Tank> Board::get_player_tank(size_t id) 
{
    auto players_it = players_.find(id);
    return (players_it != players_.end()) ? players_it->second.tank() : nullptr;
}

const std::shared_ptr<Tank> Board::get_player_tank(size_t id) const 
{
    const auto players_it = players_.find(id);
    return (players_it != players_.end()) ? players_it->second.tank() : nullptr;
}

bool Board::execute_tank_action(std::shared_ptr<Tank> tank, TankAction action) 
{
    if (!tank || !tank->is_alive()) return false;

    tank->decrease_cooldown();

    Position current_pos = tank->position();
    Position new_pos;
    
    switch (action) 
    {
        case TankAction::MoveForward: 
        {
            if constexpr (config::get<bool>("verbose_debug"))
                std::cout << "[Board] Executing MoveForward for Tank " << tank->id() << std::endl;
            
            if (tank->is_backing()) 
            {
                // Only move forward action is able to reset the back movement
                tank->reset_backwait();
                return true;
            }

            new_pos = forward_position(tank->position(), tank->direction());
            
            Cell& current_cell = grid_[tank->position().first][tank->position().second];
            Cell& new_cell = grid_[new_pos.first][new_pos.second];
            
            if (new_cell.has(ObjectType::Wall))
            {
                // Illegal move, can't move into walls
                return false;
            }
            
            new_cell.add_object(tank);
            current_cell.remove_object(tank);
            tank->position() = new_pos;
            cells_to_update_.insert(new_pos);

            // Store the old position of the tank
            // Should never be two tanks in the same position in a valid game state, so should be ok
            old_tanks_positions_[current_pos] = tank;

            return true;
        }
        case TankAction::MoveBackward: 
        {
            if constexpr (config::get<bool>("verbose_debug"))
                std::cout << "[Board] Executing MoveBackward for Tank " << tank->id() << std::endl;
        
            if (!tank->is_backing()) 
            {
                tank->start_backwait();
                return true;
            } 
            else 
            {
                tank->tick_backwait();  // keep counting
                if (tank->ready_to_move_back()) 
                {
                    tank->continue_backing();
                    new_pos = forward_position(tank->position(), static_cast<Direction>((static_cast<int>(tank->direction()) + 4) % 8));

                    Cell& current_cell = grid_[tank->position().first][tank->position().second];
                    Cell& new_cell = grid_[new_pos.first][new_pos.second];
                    
                    if (new_cell.has(ObjectType::Wall))
                    {
                        // Illegal move, can't move into walls
                        return false;
                    }
        
                    new_cell.add_object(tank);
                    current_cell.remove_object(tank);
                    tank->position() = new_pos;
                    cells_to_update_.insert(new_pos);

                    // Store the old position of the tank
                    // Should never be two tanks in the same position in a valid game state, so should be ok
                    old_tanks_positions_[current_pos] = tank;

                    return true;
                }
            }

            return true;  // still waiting
        }

        case TankAction::RotateLeft_1_8: 
        {
            if constexpr (config::get<bool>("verbose_debug"))
                std::cout << "[Board] Executing RotateLeft_1_8 for Tank " << tank->id() << std::endl;
    
            bool is_backing = tank->is_backing();
            tank->tick_backwait();
            if (is_backing) 
            {
                return false;
            }

            tank->direction() = static_cast<Direction>((static_cast<int>(tank->direction()) + 7) % 8);
            return true;
        }

        case TankAction::RotateRight_1_8: 
        {
            if constexpr (config::get<bool>("verbose_debug"))
                std::cout << "[Board] Executing RotateRight_1_8 for Tank " << tank->id() << std::endl;
    
            bool is_backing = tank->is_backing();
            tank->tick_backwait();
            if (is_backing) 
            {
                return false;
            }

            tank->direction() = static_cast<Direction>((static_cast<int>(tank->direction()) + 1) % 8);
            return true;
        }

        case TankAction::RotateLeft_1_4: 
        {
            if constexpr (config::get<bool>("verbose_debug"))
                std::cout << "[Board] Executing RotateLeft_1_4 for Tank " << tank->id() << std::endl;
            
            bool is_backing = tank->is_backing();
            tank->tick_backwait();
            if (is_backing) 
            {
                return false;
            }

            tank->direction() = static_cast<Direction>((static_cast<int>(tank->direction()) + 6) % 8);
            return true;
        }

        case TankAction::RotateRight_1_4: 
        {
            if constexpr (config::get<bool>("verbose_debug"))
                std::cout << "[Board] Executing RotateRight_1_4 for Tank " << tank->id() << std::endl;
    
            bool is_backing = tank->is_backing();
            tank->tick_backwait();
            if (is_backing) 
            {
                return false;
            }

            tank->direction() = static_cast<Direction>((static_cast<int>(tank->direction()) + 2) % 8);
            return true;
        }

        case TankAction::Shoot: 
        {
            if constexpr (config::get<bool>("verbose_debug"))
                std::cout << "[Board] Executing Shoot for Tank " << tank->id() << std::endl;
    
            tank->tick_backwait();
            if (tank->can_shoot()) 
            {
                Position shell_pos = forward_position(tank->position(), tank->direction());
                auto &cell = grid_[shell_pos.first][shell_pos.second];
                tank->shoot();
                std::shared_ptr<Shell> shell = std::make_shared<Shell>(tank->direction());
                cell.add_object(shell);
                active_shells_.emplace_back(shell_pos, shell);
                cells_to_update_.insert(shell_pos);
                return true;
            }

            return false;
        }

        case TankAction::Idle: 
        {
            if constexpr (config::get<bool>("verbose_debug"))
                std::cout << "[Board] Executing Idle for Tank " << tank->id() << std::endl;
    
            tank->tick_backwait();
    
            // Staying idle is always legal
            return true;
        }

        default:
            return false;
    }
}

// Moves all the shells one step forward, does not resolve collisions (besides crossing shells)
void Board::update_active_shells() 
{
    std::vector<std::tuple<Position, Position, std::shared_ptr<Shell>>> moves; // (from, to, shell)

    // First pass: prepeare moves
    for (const auto& [pos, shell] : active_shells_)
    {
        Position new_pos = forward_position(pos, shell->direction());
        moves.emplace_back(pos, new_pos, shell);
    }

    std::unordered_set<std::shared_ptr<Shell>> shells_to_remove;

    // Check for collisions (crossing shells)
    for (size_t i = 0; i < moves.size(); ++i)
    {
        for (size_t j = i + 1; j < moves.size(); ++j)
        {
            auto& [from1, to1, shell1] = moves[i];
            auto& [from2, to2, shell2] = moves[j];

            if (to1 == from2 && to2 == from1)
            {
                // Shells are crossing each other, should mark as collision
                shells_to_remove.insert(shell1);
                shells_to_remove.insert(shell2);
            }
        }
    }

    // Second pass: move shells on the board, excluding removed shells
    for (auto& [from, to, shell] : moves)
    {
        grid_[from.first][from.second].remove_object(shell);
        if (shells_to_remove.count(shell) == 0)
        {
            grid_[to.first][to.second].add_object(shell);
            cells_to_update_.insert(to);

            // Update the position in active_shells_
            auto it = std::find_if(active_shells_.begin(), active_shells_.end(),
                                [&](const auto& p) { return p.second == shell; });
            if (it != active_shells_.end())
            {
                it->first = to; // update position
            }
        }
        // Else: crossing shell, should be removed
    }

    // Remove crossing shells from the active shells list
    active_shells_.erase(
        std::remove_if(active_shells_.begin(), active_shells_.end(),
            [&](const auto& pair) {
                const auto& shell = pair.second;
                return shells_to_remove.count(shell) > 0;
            }),
        active_shells_.end()
    );
}

void Board::resolve_collisions(Cell& cell) 
{
    if ((((cell.has(ObjectType::Shell) && !cell.has(ObjectType::Mine)) ||
        (!cell.has(ObjectType::Shell) && cell.has(ObjectType::Mine))) &&
        cell.get_objects_count() > 1) ||
        (cell.has(ObjectType::Shell) && cell.has(ObjectType::Mine) && cell.get_objects_count() > 2))
    {
        // If there's shell or mine and at least one another object, we have an explosion
        // If there're both and at least one another object, we have an explosion
        // Want to allow shell and mine to be in the same cell without exploding
        on_explosion(cell);
    }

    else if (cell.get_objects_by_type(ObjectType::Tank).size() > 1)
    {
        // Two tanks collided, both are destroyed
        for (auto& tank : cell.get_objects_by_type(ObjectType::Tank))
        {
            auto tank_ptr = std::static_pointer_cast<Tank>(tank);
            tank_ptr->destroy();
        }

        // Remove all tanks from the cell
        cell.remove_objects_by_type(ObjectType::Tank);
    }
    
}

void Board::on_explosion(Cell& cell) 
{
    // We have an explosion, all the objects must get hurt

    std::vector<std::shared_ptr<GameObjectInterface>> objects_to_remove;

    // Weaken wall if exists
    if (cell.has(ObjectType::Wall))
    {
        for (auto& wall : cell.get_objects_by_type(ObjectType::Wall))
        {
            auto wall_ptr = std::static_pointer_cast<Wall>(wall);
            wall_ptr->weaken();
            if (wall_ptr->is_destroyed())
            {
                objects_to_remove.push_back(wall);
            }
        }
    }

    // Destroy all tanks
    if (cell.has(ObjectType::Tank))
    {
        for (auto& tank : cell.get_objects_by_type(ObjectType::Tank))
        {
            auto tank_ptr = std::static_pointer_cast<Tank>(tank);
            tank_ptr->destroy();
            objects_to_remove.push_back(tank);
        }
    }

    // Destroy all shells
    if (cell.has(ObjectType::Shell))
    {
        for (auto& shell : cell.get_objects_by_type(ObjectType::Shell))
        {
            auto shell_ptr = std::static_pointer_cast<Shell>(shell);

            // Remove the shell from the active shells list
            Position shell_position = cell.position();
            auto it = std::find_if(active_shells_.begin(), active_shells_.end(),
                [&shell_position, &shell_ptr](const std::pair<Position, std::shared_ptr<Shell>>& active_shell)
                {
                    return active_shell.first == shell_position && active_shell.second == shell_ptr;
                }
            );
            if (it != active_shells_.end())
            {
                active_shells_.erase(it);
            }

            // Mark for removal from the cell
            objects_to_remove.push_back(shell);
        }
    }

    // Destroy mine if exists
    if (cell.has(ObjectType::Mine))
    {
        for (auto& mine : cell.get_objects_by_type(ObjectType::Mine))
        {
            objects_to_remove.push_back(mine);
        }
    }

    // Remove all collected objects safely, to avoid invalidating iterators
    for (auto& object : objects_to_remove)
    {
        cell.remove_object(object);
    }
}

void Board::do_shells_step() 
{
    // Move all the shells one step forward
    update_active_shells();

    // Update the board and resolve collisions
    update();
}

void Board::update() 
{
    // Check for crossing tanks
    for (auto it1 = old_tanks_positions_.begin(); it1 != old_tanks_positions_.end(); ++it1)
    {
        for (auto it2 = std::next(it1); it2 != old_tanks_positions_.end(); ++it2)
        {
            Position old_pos1 = it1->first;
            auto tank1 = it1->second;

            Position old_pos2 = it2->first;
            auto tank2 = it2->second;

            if (tank1->position() == old_pos2 && tank2->position() == old_pos1)
            {
                // Tanks are crossing each other, both should be destroyed
                tank1->destroy();
                grid_[tank1->position().first][tank1->position().second].remove_object(tank1);

                tank2->destroy();
                grid_[tank2->position().first][tank2->position().second].remove_object(tank2);
            }
        }
    }

    // Clear the old tanks positions for the next turn
    old_tanks_positions_.clear();
    
    // Resolve collisions for all the cells from this turn
    for (auto& cell_pos : cells_to_update_)
    {
        resolve_collisions(grid_[cell_pos.first][cell_pos.second]);
    }

    // Clear the cells to update set for the next turn
    cells_to_update_.clear();
}

Position Board::forward_position(const Position &pos, Direction dir) const 
{
    static const std::unordered_map<Direction, std::pair<int, int>> deltas = {
        {Direction::U, {0, -1}}, {Direction::UR, {1, -1}}, {Direction::R, {1, 0}},  {Direction::DR, {1, 1}},
        {Direction::D, {0, 1}},  {Direction::DL, {-1, 1}}, {Direction::L, {-1, 0}}, {Direction::UL, {-1, -1}} };
    auto [dx, dy] = deltas.at(dir);
    int new_x = (pos.first + dx + width_) % width_;
    int new_y = (pos.second + dy + height_) % height_;

    return Position(new_x, new_y);
}

const Cell& Board::get_cell(Position position) const
{
    int x = position.first % width_;
    int y = position.second % height_;
    return grid_[x][y];
}

size_t Board::get_height() const 
{
    return height_;
}

size_t Board::get_width() const 
{
    return width_;
}
