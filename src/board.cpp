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

#include "algorithms/aggressive_chase_algorithm.h"
#include "algorithms/smart_algorithm.h"
#include "algorithms/seed_algorithm.h"

std::map<size_t, Player>& Board::players() {
    return players_;
}

bool Board::load_from_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) return false;

    file >> width_ >> height_;
    file.ignore();  // skip newline

    grid_.resize(height_, std::vector<Cell>(width_));

    std::string line;
    for (size_t y = 0; y < height_; ++y) {
        std::getline(file, line);
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
                    // TODO: move the algorithms to the static config, read it based on name
                    auto algo = std::make_shared<SeedAlgorithm>(std::initializer_list<TankAction>{
                        TankAction::MoveBackward, TankAction::Shoot, TankAction::Idle, TankAction::Shoot, TankAction::MoveBackward});
                    auto tank = std::make_shared<Tank>(1, pos, Direction::L);
                    auto player = Player(tank, algo);
                    players_[1] = player;
                    grid_[x][y] = Cell(pos, tank);
                    break;
                }
                case '2': {
                    // TODO: make better...
                    auto tank = std::make_shared<Tank>(2, pos, Direction::R);
                    auto algo = std::make_shared<AggressiveChaseAlgorithm>();
                    auto player = Player(tank, algo);
                    players_[2] = player;
                    grid_[x][y] = Cell(pos, tank);
                    break;
                }
                default: {
                    grid_[x][y] = Cell(pos);
                    break;
                }
            }
        }
    }
    return true;
}

void Board::print() const {
    // TODO: change back to std::cout
    // TODO: add general try catch

    std::cerr << "Game Board:\n";

    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            Position p(x, y);
            if (!grid_[x][y].objects().empty()) {
                auto &[type, object] = *grid_[x][y].objects().begin();
                switch (type) {
                    case ObjectType::Wall:
                        std::cerr << '#';
                        break;
                    case ObjectType::Mine:
                        std::cerr << '@';
                        break;
                    case ObjectType::Tank: {
                        auto* t = static_cast<Tank *>(object.get());
                        std::cerr << (t->id() == 1 ? '1' : '2');
                        break;
                    }
                    case ObjectType::Shell:
                        std::cerr << '*';
                        break;
                }
            } else {
                std::cerr << ' ';
            }
        }
        std::cerr << "\n";
    }
}

Tank* Board::get_player_tank(size_t id) {
    assert(players_.contains(id));
    auto players_it = players_.find(id);
    return players_it->second.tank();
}

const Tank* Board::get_player_tank(size_t id) const {
    assert(players_.contains(id));
    const auto players_it = players_.find(id);
    return players_it->second.tank();
}

bool Board::execute_tank_action(Tank *tank, TankAction action) {
    if (!tank || !tank->is_alive()) return false;

    tank->decrease_cooldown();

    Position newPos;
    switch (action) {
        case TankAction::MoveForward:
            if (tank->is_backing()) {
                // Only move forward action is able to reset the back movement
                tank->reset_backwait();
                return true;
            }
            newPos = forward_position(tank->position(), tank->direction());

            if (grid_[newPos.first][newPos.second].empty()) {
                grid_[tank->position().first][tank->position().second].remove_object(ObjectType::Tank);
                tank->position() = newPos;
                grid_[newPos.first][newPos.second].add_object(std::shared_ptr<Tank>(players_[tank->id()].tank()));
                return true;
            } else if (grid_[newPos.first][newPos.second].has(ObjectType::Mine)) {
                grid_[newPos.first][newPos.second].remove_object(ObjectType::Mine);
                tank->destroy();
                return true;
            } else if (grid_[newPos.first][newPos.second].has(ObjectType::Tank)) {
                // TODO change to static_pointer_cast
                auto targetTank = std::dynamic_pointer_cast<Tank>(grid_[newPos.first][newPos.second].get_object(ObjectType::Tank));
                targetTank->destroy();
                grid_[newPos.first][newPos.second].remove_object(ObjectType::Tank);
                tank->destroy();
                return true;
            }

            return false;

        case TankAction::MoveBackward:
            if (!tank->is_backing()) {
                tank->start_backwait();
                return true;
            } else {
                tank->tick_backwait();  // keep counting
                if (tank->ready_to_move_back()) {
                    tank->continue_backing();
                    newPos = forward_position(tank->position(), static_cast<Direction>((static_cast<int>(tank->direction()) + 4) % 8));
                    if (grid_[newPos.first][newPos.second].empty()) {
                        grid_[tank->position().first][tank->position().second].remove_object(ObjectType::Tank);
                        tank->position() = newPos;
                        grid_[newPos.first][newPos.second].add_object(std::shared_ptr<Tank>(players_[tank->id()].tank()));
                        return true;
                    } else if (grid_[newPos.first][newPos.second].has(ObjectType::Mine)) {
                        grid_[newPos.first][newPos.second].remove_object(ObjectType::Mine);
                        tank->destroy();
                        return true;
                    } else if (grid_[newPos.first][newPos.second].has(ObjectType::Tank)) {
                        // TODO change to static_pointer_cast
                        auto targetTank = std::dynamic_pointer_cast<Tank>(grid_[newPos.first][newPos.second].get_object(ObjectType::Tank));
                        targetTank->destroy();
                        grid_[newPos.first][newPos.second].remove_object(ObjectType::Tank);
                        tank->destroy();
                        return true;
                    }

                    return false;
                }
            }

            return true;  // still waiting

        case TankAction::RotateLeft_1_8:  // TODO: fallthrough magic
            if (!tank->is_backing()) tank->direction() = static_cast<Direction>((static_cast<int>(tank->direction()) + 1) % 8);
            tank->tick_backwait();
            return true;

        case TankAction::RotateRight_1_8:
            if (!tank->is_backing()) tank->direction() = static_cast<Direction>((static_cast<int>(tank->direction()) + 7) % 8);
            tank->tick_backwait();
            return true;

        case TankAction::RotateLeft_1_4:
            if (!tank->is_backing()) tank->direction() = static_cast<Direction>((static_cast<int>(tank->direction()) + 2) % 8);
            tank->tick_backwait();
            return true;

        case TankAction::RotateRight_1_4:
            if (!tank->is_backing()) tank->direction() = static_cast<Direction>((static_cast<int>(tank->direction()) + 6) % 8);
            tank->tick_backwait();
            return true;

        case TankAction::Shoot:
            tank->tick_backwait();
            if (tank->can_shoot()) {
                Position shellPos = forward_position(tank->position(), tank->direction());
                auto &cell = grid_[shellPos.first][shellPos.second];
                if (cell.empty()) {
                    tank->shoot();
                    cell.add_object(std::make_shared<Shell>(tank->direction()));
                    return true;
                } else if (cell.has(ObjectType::Wall)) {
                    auto wall = std::dynamic_pointer_cast<Wall>(cell.get_object(ObjectType::Wall));
                    wall->weaken();
                    tank->shoot();
                    if (wall->is_destroyed()) {
                        cell.remove_object(ObjectType::Wall);
                    }

                    return true;
                } else if (cell.has(ObjectType::Tank)) {
                    auto targetTank = std::dynamic_pointer_cast<Tank>(cell.get_object(ObjectType::Tank));
                    targetTank->destroy();
                    cell.remove_object(ObjectType::Tank);
                    tank->shoot();
                    return true;
                }
                // ignore shell hitting mine
            }
            return false;

        case TankAction::Idle:
            tank->tick_backwait();
            return true;

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
                auto shell = std::dynamic_pointer_cast<Shell>(cell.get_object(ObjectType::Shell));
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
                            auto wall = std::dynamic_pointer_cast<Wall>(new_cell.get_object(ObjectType::Wall));
                            wall->weaken();
                            if (wall->is_destroyed()) {
                                new_cell.remove_object(ObjectType::Wall);
                            }
                        } else if (new_cell.has(ObjectType::Tank)) {
                            auto tank = std::dynamic_pointer_cast<Tank>(new_cell.get_object(ObjectType::Tank));
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
