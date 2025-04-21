#pragma once

#include <unordered_map>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <map>

#include "game_object_interface.h"
#include "types/position.h"
#include "types/direction.h"
#include "types/tank_action.h"

#include "cell.h"
#include "tank.h"
#include "wall.h"
#include "mine.h"
#include "shell.h"

class Board
{
    // TODO: separate to public and private
    int width_, height_;
    std::vector<std::vector<Cell>> grid_;
    std::shared_ptr<Tank> tank1_ = nullptr;
    std::shared_ptr<Tank> tank2_ = nullptr;

public:
    Board() = default;

    bool loadFromFile(const std::string &filename)
    {
        std::ifstream file(filename);
        if (!file)
            return false;

        file >> width_ >> height_;
        file.ignore(); // skip newline

        grid_.resize(height_, std::vector<Cell>(width_));

        std::string line;
        for (int y = 0; y < height_; ++y)
        {
            std::getline(file, line);
            for (int x = 0; x < width_ && x < (int)line.size(); ++x)
            {
                char ch = line[x];
                Position pos(x, y);
                switch (ch)
                {
                case '#':
                    grid_[x][y] = Cell(pos, std::make_shared<Wall>());
                    break;
                case '@':
                    grid_[x][y] = Cell(pos, std::make_shared<Mine>());
                    break;
                case '1':
                    tank1_ = std::make_shared<Tank>(1, pos, Direction::L);
                    grid_[x][y] = Cell(pos, tank1_);
                    break;
                case '2':
                    tank2_ = std::make_shared<Tank>(2, pos, Direction::R);
                    grid_[x][y] = Cell(pos, tank2_);
                    break;
                default:
                    grid_[x][y] = Cell(pos);
                    break;
                }
            }
        }
        return true;
    }

    void print() const
    {
        // TODO: change back to std::cout

        std::cerr << "Game Board:\n";

        for (int y = 0; y < height_; ++y)
        {
            for (int x = 0; x < width_; ++x)
            {
                Position p(x, y);
                if (grid_[x][y].object) {
                    switch (grid_[x][y].object->type()) {
                        case ObjectType::Wall:
                            std::cerr << '#';
                            break;
                        case ObjectType::Mine:
                            std::cerr << '@';
                            break;
                        case ObjectType::Tank: {
                            auto *t = dynamic_cast<Tank *>(grid_[x][y].object.get());
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

    Tank *getPlayerTank(int id) const { return (id == 1 ? tank1_ : tank2_).get(); }
    bool executeTankAction(Tank *tank, TankAction action) {
        if (!tank || !tank->isAlive()) return false;

        tank->decreaseCooldown();
        tank->tickBackwait();

        Position newPos;
        switch (action) {
            case TankAction::MoveForward:
                if (tank->isBacking()) {
                    return false;  // ignore other actions during backwait unless canceling
                }
                newPos = forwardPos(tank->pos, tank->dir);
                if (grid_[newPos.first][newPos.second].object == nullptr) {
                    grid_[tank->pos.first][tank->pos.second].object = nullptr;
                    tank->pos = newPos;
                    grid_[newPos.first][newPos.second].object = tank1_.get() == tank ? tank1_ : tank2_;
                    return true;
                } else if (grid_[newPos.first][newPos.second].object->type() == ObjectType::Mine) {
                    grid_[newPos.first][newPos.second].object = nullptr;
                    tank->destroy();
                    return true;
                } else if (grid_[newPos.first][newPos.second].object->type() == ObjectType::Tank) {
                    auto targetTank = std::dynamic_pointer_cast<Tank>(grid_[newPos.first][newPos.second].object);
                    targetTank->destroy();
                    grid_[newPos.first][newPos.second].object = nullptr;
                    tank->destroy();
                    return true;
                }

                return false;

            case TankAction::MoveBackward:
                if (!tank->isBacking()) {
                    tank->startBackwait();
                    return true;
                } else if (tank->readyToMoveBack()) {
                    newPos = forwardPos(tank->pos, static_cast<Direction>((static_cast<int>(tank->dir) + 4) % 8));
                    if (grid_[newPos.first][newPos.second].object == nullptr) {
                        grid_[tank->pos.first][tank->pos.second].object = nullptr;
                        tank->pos = newPos;
                        grid_[newPos.first][newPos.second].object = tank1_.get() == tank ? tank1_ : tank2_;
                        return true;
                    } else if (grid_[newPos.first][newPos.second].object->type() == ObjectType::Mine) {
                        grid_[newPos.first][newPos.second].object = nullptr;
                        tank->destroy();
                        return true;
                    } else if (grid_[newPos.first][newPos.second].object->type() == ObjectType::Tank) {
                        auto targetTank = std::dynamic_pointer_cast<Tank>(grid_[newPos.first][newPos.second].object);
                        targetTank->destroy();
                        grid_[newPos.first][newPos.second].object = nullptr;
                        tank->destroy();
                        return true;
                    }

                    return false;
                }

                return true;  // still waiting

            case TankAction::RotateLeft_1_8:
                if (!tank->isBacking()) tank->dir = static_cast<Direction>((static_cast<int>(tank->dir) + 1) % 8);
                return true;

            case TankAction::RotateRight_1_8:
                if (!tank->isBacking()) tank->dir = static_cast<Direction>((static_cast<int>(tank->dir) + 7) % 8);
                return true;

            case TankAction::RotateLeft_1_4:
                if (!tank->isBacking()) tank->dir = static_cast<Direction>((static_cast<int>(tank->dir) + 2) % 8);
                return true;

            case TankAction::RotateRight_1_4:
                if (!tank->isBacking()) tank->dir = static_cast<Direction>((static_cast<int>(tank->dir) + 6) % 8);
                return true;

            case TankAction::Shoot:
                if (tank->canShoot()) {
                    Position shellPos = forwardPos(tank->pos, tank->dir);
                    auto &cell = grid_[shellPos.first][shellPos.second];
                    if (cell.object == nullptr) {
                        tank->shoot();
                        cell.object = std::make_shared<Shell>(tank->dir);
                        return true;
                    } else if (cell.object->type() == ObjectType::Wall) {
                        auto wall = std::dynamic_pointer_cast<Wall>(cell.object);
                        wall->weaken();
                        tank->shoot();
                        if (wall->isDestroyed()) cell.object = nullptr;
                        return true;
                    } else if (cell.object->type() == ObjectType::Tank) {
                        auto targetTank = std::dynamic_pointer_cast<Tank>(cell.object);
                        targetTank->destroy();
                        cell.object = nullptr;
                        tank->shoot();
                        return true;
                    }
                    // ignore shell hitting mine
                }
                return false;

            case TankAction::Idle:
                return true;

            default:
                return false;
        }
    }

    void updateShells() {
        std::vector<std::pair<Position, std::shared_ptr<Shell>>> new_shells;

        for (size_t i = 0; i < grid_.size(); ++i) {
            for (size_t j = 0; j < grid_[i].size(); ++j) {
                auto &cell = grid_[i][j];
                if (cell.object && cell.object->type() == ObjectType::Shell) {
                    auto shell = std::dynamic_pointer_cast<Shell>(cell.object);
                    Position mid_pos = forwardPos(Position(i, j), shell->dir);
                    Position end_pos = forwardPos(mid_pos, shell->dir);

                    std::map<Position, bool> states;
                    states[mid_pos] = false;
                    states[end_pos] = true;

                    for (const auto &[step_pos, add_board_indication] : states) {
                        auto &new_cell = grid_[step_pos.first][step_pos.second];
                        if (!new_cell.object) {
                            if (add_board_indication) {
                                new_shells.emplace_back(step_pos, shell);
                            }
                        } else {
                            auto obj_type = new_cell.object->type();
                            if (obj_type == ObjectType::Wall) {
                                auto wall = std::dynamic_pointer_cast<Wall>(new_cell.object);
                                wall->weaken();
                                if (wall->isDestroyed()) new_cell.object = nullptr;
                            } else if (obj_type == ObjectType::Tank) {
                                auto tank = std::dynamic_pointer_cast<Tank>(new_cell.object);
                                tank->destroy();
                                new_cell.object = nullptr;
                            } else if (obj_type == ObjectType::Shell) {
                                new_cell.object = nullptr;  // both shells explode
                            }
                        }
                        cell.object = nullptr;  // remove the shell from its original place
                    }
                }
            }
        }

        for (auto &[new_pos, shell] : new_shells) {
            grid_[new_pos.first][new_pos.second] = Cell(new_pos, shell);
        }
    }

    void update() {
        updateShells();
    }

    Position forwardPos(const Position &pos, Direction dir) const {
        static const std::unordered_map<Direction, std::pair<int, int>> deltas = {
            {Direction::U, {0, -1}}, {Direction::UR, {1, -1}}, {Direction::R, {1, 0}},  {Direction::DR, {1, 1}},
            {Direction::D, {0, 1}},  {Direction::DL, {-1, 1}}, {Direction::L, {-1, 0}}, {Direction::UL, {-1, -1}}};
        auto [dx, dy] = deltas.at(dir);
        int new_x = (pos.first + dx + width_) % width_;
        int new_y = (pos.second + dy + height_) % height_;
        return Position(new_x, new_y);
    }
};
