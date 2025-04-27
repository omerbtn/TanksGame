#include "board.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <map>
#include <cassert>

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

std::map<size_t, Player>& Board::players() {
    return players_;
}

std::vector<std::vector<Cell>>& Board::grid() {
    return grid_;
}

const std::string& Board::input_file_name() const {
    return input_file_name_;
}

bool Board::load_from_file(const std::string& filename) {
    input_file_name_ = filename;

    InputErrorLogger error_logger;
    std::ifstream file(filename);
    if (!file) {
        error_logger.log("Couldn't open file: ", filename);
        return false;
    }

    file >> width_ >> height_;
    file.ignore();  // skip newline

    grid_.resize(height_, std::vector<Cell>(width_));

    std::string line;
    for (size_t y = 0; y < height_; ++y) {
        std::getline(file, line);
        if (line.size() != width_) {
            error_logger.log("Warning: The width of the ", y, " row is wrong. Filling the missing cells and ignoring the extra cells.");
        }

        for (size_t x = 0; x < width_ && x < line.size(); ++x) {
            char ch = line[x];
            Position pos(x, y);
            switch (ch) {
                case '#': {
                    grid_[x][y] = Cell(pos, std::make_shared<Wall>());
                    break;
                }
                case '@': {
                    grid_[x][y] = Cell(pos, std::make_shared<Mine>());
                    break;
                }
                case '1': {
                    if (players_.contains(1)) {
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
                case '2': {
                    if (players_.contains(2)) {
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
                case '.': {
                    grid_[x][y] = Cell(pos);
                    break;
                }
                default: {
                    // Unexpected character
                    error_logger.log("Warning: invalid character '", ch, "' at (", x, ",", y, "). Treating as empty.");
                    grid_[x][y] = Cell(pos);
                    break;
                }
            }
        }
    }

    if (players_.size() != config::get<size_t>("num_players")) {
        error_logger.log("Warning: Not enough players. Aborting.");
        return false;
    }

    return true;
}

void Board::print() const {
    std::cout << "Game Board:\n";

    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            Position p(x, y);
            std::string to_print = "[";
            if (!grid_[x][y].objects().empty())
            {
                for (auto &[type, object] : grid_[x][y].objects())
                {
                    switch (type) {
                        case ObjectType::Wall:
                            to_print += '#';
                            break;
                        case ObjectType::Mine:
                            to_print += '@';
                            break;
                        case ObjectType::Tank: {
                            auto tank = std::static_pointer_cast<Tank>(object);
                            to_print += (tank->id() == 1 ? '1' : '2');
                            to_print += directionToArrow(tank->direction());
                            break;
                        }
                        case ObjectType::Shell:
                            to_print += '*';
                            break;
                    }
                }
            }
            for (int i = to_print.size(); i < 4; ++i) {
                to_print += " ";
            }
            to_print += "]";
            std::cout << to_print;
        }
        std::cout << "\n";
    }
}

std::shared_ptr<Tank> Board::get_player_tank(size_t id) {
    assert(players_.contains(id));
    auto players_it = players_.find(id);
    return players_it->second.tank();
}

const std::shared_ptr<Tank> Board::get_player_tank(size_t id) const {
    assert(players_.contains(id));
    const auto players_it = players_.find(id);
    return players_it->second.tank();
}

bool Board::execute_tank_action(std::shared_ptr<Tank> tank, TankAction action) {
    if (!tank || !tank->is_alive()) return false;

    tank->decrease_cooldown();

    Position newPos;
    switch (action) {
        case TankAction::MoveForward: {
            std::cout << "[Board] Executing MoveForward for Tank " << tank->id() << std::endl;
            if (tank->is_backing()) {
                // Only move forward action is able to reset the back movement
                tank->reset_backwait();
                return true;
            }
            newPos = forward_position(tank->position(), tank->direction());

            if (grid_[newPos.first][newPos.second].empty()) {
                if constexpr (config::get<bool>("verbose_debug")) {
                    std::cout << "[Board] Moving Tank " << tank->id() << " to empty cell" << std::endl;
                    std::cout << "[Board] Adding Tank " << tank->id() << " to position (" << newPos.first << "," << newPos.second << ")"
                              << std::endl;
                }
                grid_[newPos.first][newPos.second].add_object(std::shared_ptr<Tank>(players_[tank->id()].tank()));
                if constexpr (config::get<bool>("verbose_debug")) {
                    std::cout << "[Board] Removing Tank " << tank->id() << " from position (" << tank->position().first << ","
                              << tank->position().second << ")" << std::endl;
                }
                grid_[tank->position().first][tank->position().second].remove_object(ObjectType::Tank);
                tank->position() = newPos;
                if constexpr (config::get<bool>("verbose_debug")) {
                    std::cout << "[Board] Tank " << tank->id() << " moved to (" << newPos.first << "," << newPos.second << ")" << std::endl;
                }
                return true;
            } else if (grid_[newPos.first][newPos.second].has(ObjectType::Mine)) {
                grid_[newPos.first][newPos.second].remove_object(ObjectType::Mine);
                tank->destroy();
                return true;
            } else if (grid_[newPos.first][newPos.second].has(ObjectType::Tank)) {
                auto targetTank = std::static_pointer_cast<Tank>(grid_[newPos.first][newPos.second].get_object(ObjectType::Tank));
                targetTank->destroy();
                grid_[newPos.first][newPos.second].remove_object(ObjectType::Tank);
                tank->destroy();
                return true;
            }

            return false;
        }
        case TankAction::MoveBackward: {
            std::cout << "[Board] Executing MoveBackward for Tank " << tank->id() << std::endl;
            if (!tank->is_backing()) {
                tank->start_backwait();
                return true;
            } else {
                tank->tick_backwait();  // keep counting
                if (tank->ready_to_move_back()) {
                    tank->continue_backing();
                    newPos = forward_position(tank->position(), static_cast<Direction>((static_cast<int>(tank->direction()) + 4) % 8));
                    if (grid_[newPos.first][newPos.second].empty()) {
                        grid_[newPos.first][newPos.second].add_object(std::shared_ptr<Tank>(players_[tank->id()].tank()));
                        grid_[tank->position().first][tank->position().second].remove_object(ObjectType::Tank);
                        tank->position() = newPos;
                        return true;
                    } else if (grid_[newPos.first][newPos.second].has(ObjectType::Mine)) {
                        grid_[newPos.first][newPos.second].remove_object(ObjectType::Mine);
                        tank->destroy();
                        return true;
                    } else if (grid_[newPos.first][newPos.second].has(ObjectType::Tank)) {
                        auto targetTank = std::static_pointer_cast<Tank>(grid_[newPos.first][newPos.second].get_object(ObjectType::Tank));
                        targetTank->destroy();
                        grid_[newPos.first][newPos.second].remove_object(ObjectType::Tank);
                        tank->destroy();
                        return true;
                    }

                    return false;
                }
            }

            return true;  // still waiting
        }
        case TankAction::RotateLeft_1_8: {
            std::cout << "[Board] Executing RotateLeft_1_8 for Tank " << tank->id() << std::endl;
            bool is_backing = tank->is_backing();
            tank->tick_backwait();
            if (is_backing) {
                return false;
            }

            tank->direction() = static_cast<Direction>((static_cast<int>(tank->direction()) + 7) % 8);
            return true;
        }

        case TankAction::RotateRight_1_8: {
            std::cout << "[Board] Executing RotateRight_1_8 for Tank " << tank->id() << std::endl;
            bool is_backing = tank->is_backing();
            tank->tick_backwait();
            if (is_backing) {
                return false;
            }

            tank->direction() = static_cast<Direction>((static_cast<int>(tank->direction()) + 1) % 8);
            return true;
        }

        case TankAction::RotateLeft_1_4: {
            std::cout << "[Board] Executing RotateLeft_1_4 for Tank " << tank->id() << std::endl;
            bool is_backing = tank->is_backing();
            tank->tick_backwait();
            if (is_backing) {
                return false;
            }

            tank->direction() = static_cast<Direction>((static_cast<int>(tank->direction()) + 6) % 8);
            return true;
        }
        case TankAction::RotateRight_1_4: {
            std::cout << "[Board] Executing RotateRight_1_4 for Tank " << tank->id() << std::endl;
            bool is_backing = tank->is_backing();
            tank->tick_backwait();
            if (is_backing) {
                return false;
            }

            tank->direction() = static_cast<Direction>((static_cast<int>(tank->direction()) + 2) % 8);
            return true;
        }

        case TankAction::Shoot: {
            std::cout << "[Board] Executing Shoot for Tank " << tank->id() << std::endl;
            tank->tick_backwait();
            if (tank->can_shoot()) {
                auto &cell = grid_[tank->position().first][tank->position().second];
                tank->shoot();
                cell.add_object(std::make_shared<Shell>(tank->direction()));
                return true;
            }
            return false;
        }

        case TankAction::Idle: {
            std::cout << "[Board] Executing Idle for Tank " << tank->id() << std::endl;
            bool is_backing = tank->is_backing();
            tank->tick_backwait();
            if (is_backing) {
                return false;
            }

            return true;
        }

        default:
            return false;
    }
}

void Board::update_shells() {
    std::vector<std::pair<Position, std::shared_ptr<Shell>>> new_shells;

    for (size_t i = 0; i < grid_.size(); ++i) {
        for (size_t j = 0; j < grid_[i].size(); ++j) {
            auto &cell = grid_[i][j];
            if (cell.has(ObjectType::Shell)) {
                auto shell = std::static_pointer_cast<Shell>(cell.get_object(ObjectType::Shell));
                Position mid_pos = forward_position(Position(i, j), shell->direction());
                Position end_pos = forward_position(mid_pos, shell->direction());

                std::map<Position, bool> states;
                states[mid_pos] = false;
                states[end_pos] = true;

                for (const auto &[step_pos, add_board_indication] : states) {
                    auto &new_cell = grid_[step_pos.first][step_pos.second];
                    if (new_cell.empty()) {
                        if (add_board_indication) {
                            new_shells.emplace_back(step_pos, shell);
                        }
                    } else {
                        if (new_cell.has(ObjectType::Wall)) {
                            auto wall = std::static_pointer_cast<Wall>(new_cell.get_object(ObjectType::Wall));
                            wall->weaken();
                            if (wall->is_destroyed()) {
                                new_cell.remove_object(ObjectType::Wall);
                            }
                        } else if (new_cell.has(ObjectType::Tank)) {
                            auto tank = std::static_pointer_cast<Tank>(new_cell.get_object(ObjectType::Tank));
                            tank->destroy();
                            new_cell.remove_object(ObjectType::Tank);
                        } else if (new_cell.has(ObjectType::Shell)) {
                            new_cell.remove_object(ObjectType::Shell);  // both shells explode
                        }
                    }
                    cell.remove_object(ObjectType::Shell);  // remove the shell from its original place
                }
            }
        }
    }

    for (const auto &[pos, shell] : new_shells) {
        grid_[pos.first][pos.second].add_object(shell);
    }
}

void Board::update() {
    update_shells();
}

Position Board::forward_position(const Position &pos, Direction dir) const {
    static const std::unordered_map<Direction, std::pair<int, int>> deltas = {
        {Direction::U, {0, -1}}, {Direction::UR, {1, -1}}, {Direction::R, {1, 0}},  {Direction::DR, {1, 1}},
        {Direction::D, {0, 1}},  {Direction::DL, {-1, 1}}, {Direction::L, {-1, 0}}, {Direction::UL, {-1, -1}}};
    auto [dx, dy] = deltas.at(dir);
    int new_x = (pos.first + dx + width_) % width_;
    int new_y = (pos.second + dy + height_) % height_;

    return Position(new_x, new_y);
}

const Cell& Board::get_cell(Position position) const
{
    int x = position.first, y = position.second;
    assert(x >= 0 && x < width_ && y >= 0 && y < height_);
    return grid_[x][y];
}

size_t Board::get_height() const {
    return height_;
}

size_t Board::get_width() const {
    return width_;
}
