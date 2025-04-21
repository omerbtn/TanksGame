#pragma once

#include <unordered_map>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

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
    int width_, height_;
    std::unordered_map<Position, Cell> grid_;
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
                    grid_[pos] = Cell(pos, std::make_shared<Wall>());
                    break;
                case '@':
                    grid_[pos] = Cell(pos, std::make_shared<Mine>());
                    break;
                case '1':
                    tank1_ = std::make_shared<Tank>(1, pos, Direction::L);
                    grid_[pos] = Cell(pos, tank1_);
                    break;
                case '2':
                    tank2_ = std::make_shared<Tank>(2, pos, Direction::R);
                    grid_[pos] = Cell(pos, tank2_);
                    break;
                default:
                    grid_[pos] = Cell(pos);
                    break;
                }
            }
        }
        return true;
    }

    void print() const
    {
        for (int y = 0; y < height_; ++y)
        {
            for (int x = 0; x < width_; ++x)
            {
                Position p(x, y);
                auto it = grid_.find(p);
                if (it != grid_.end() && it->second.object)
                {
                    switch (it->second.object->type())
                    {
                    case ObjectType::Wall:
                        std::cout << '#';
                        break;
                    case ObjectType::Mine:
                        std::cout << '@';
                        break;
                    case ObjectType::Tank:
                    {
                        auto *t = dynamic_cast<Tank *>(it->second.object.get());
                        std::cout << (t->id() == 1 ? '1' : '2');
                        break;
                    }
                    case ObjectType::Shell:
                        std::cout << '*';
                        break;
                    }
                }
                else
                    std::cout << ' ';
            }
            std::cout << "\n";
        }
    }

    Tank *getPlayerTank(int id) const { return (id == 1 ? tank1_ : tank2_).get(); }
    bool executeTankAction(Tank *self, TankAction action)
    {
        switch (action)
        {
        case TankAction::MoveForward:
        {
            Position newPos = self->pos;
            switch (self->dir)
            {
            case Direction::U:
                newPos.second--;
                break;
            case Direction::UR:
                newPos.first++;
                newPos.second--;
                break;
            case Direction::R:
                newPos.first++;
                break;
            case Direction::DR:
                newPos.first++;
                newPos.second++;
                break;
            case Direction::D:
                newPos.second++;
                break;
            case Direction::DL:
                newPos.first--;
                newPos.second++;
                break;
            case Direction::L:
                newPos.first--;
                break;
            case Direction::UL:
                newPos.first--;
                newPos.second--;
                break;
            }

            newPos.first = (newPos.first + width_) % width_;
            newPos.second = (newPos.second + height_) % height_;

            if (grid_.find(newPos) == grid_.end() || grid_[newPos].object == nullptr)
            {
                self->pos = newPos;
                return true;
            }
            return false; // Invalid move
        }
        case TankAction::MoveBackward:
        {
            Position newPos = self->pos;
            switch (self->dir)
            {
            case Direction::U:
                newPos.second++;
                break;
            case Direction::UR:
                newPos.first--;
                newPos.second++;
                break;
            case Direction::R:
                newPos.first--;
                break;
            case Direction::DR:
                newPos.first--;
                newPos.second--;
                break;
            case Direction::D:
                newPos.second--;
                break;
            case Direction::DL:
                newPos.first++;
                newPos.second--;
                break;
            case Direction::L:
                newPos.first++;
                break;
            case Direction::UL:
                newPos.first++;
                newPos.second++;
                break;
            }

            newPos.first = (newPos.first + width_) % width_;
            newPos.second = (newPos.second + height_) % height_;

            if (grid_.find(newPos) == grid_.end() || grid_[newPos].object == nullptr)
            {
                self->pos = newPos;
                // TODO: add cooldown for 2 turns
                return true;
            }
            return false; // Invalid move
        }
        case TankAction::RotateLeft_1_8:
            self->dir = static_cast<Direction>((static_cast<int>(self->dir) + 1) % 8);
            return true;
        case TankAction::RotateRight_1_8:
            self->dir = static_cast<Direction>((static_cast<int>(self->dir) - 1 + 8) % 8);
            return true;
        case TankAction::RotateLeft_1_4:
            self->dir = static_cast<Direction>((static_cast<int>(self->dir) + 2) % 8);
            return true;
        case TankAction::RotateRight_1_4:
            self->dir = static_cast<Direction>((static_cast<int>(self->dir) - 2 + 8) % 8);
            return true;
        case TankAction::Shoot:
            if (self->canShoot())
            {
                Position shellPos = self->pos;
                switch (self->dir)
                {
                case Direction::U:
                    shellPos.second--;
                    break;
                case Direction::UR:
                    shellPos.first++;
                    shellPos.second--;
                    break;
                case Direction::R:
                    shellPos.first++;
                    break;
                case Direction::DR:
                    shellPos.first++;
                    shellPos.second++;
                    break;
                case Direction::D:
                    shellPos.second++;
                    break;
                case Direction::DL:
                    shellPos.first--;
                    shellPos.second++;
                    break;
                case Direction::L:
                    shellPos.first--;
                    break;
                case Direction::UL:
                    shellPos.first--;
                    shellPos.second--;
                    break;
                }

                shellPos.first = (shellPos.first + width_) % width_;
                shellPos.second = (shellPos.second + height_) % height_;

                if (grid_.find(shellPos) == grid_.end() || grid_[shellPos].object == nullptr)
                {
                    self->shoot();
                    grid_[shellPos] = Cell(shellPos, std::make_shared<Shell>(self->dir));
                    return true;
                }
            }
            return false; // Invalid shoot
        case TankAction::Idle:
            return true; // No action taken
        default:
            return false; // Invalid action
        }
    }

    void update()
    {
        grid_[tank1_->pos] = Cell(tank1_->pos, tank1_);
        grid_[tank2_->pos] = Cell(tank2_->pos, tank2_);
        //
    }
};