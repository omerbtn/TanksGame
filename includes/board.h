#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>

#include "types/position.h"
#include "types/direction.h"
#include "types/tank_action.h"

#include "cell.h"
#include "player.h"

class Tank;

class Board
{
public:
    Board() = default;

    bool load_from_file(const std::string& filename);

    void print() const;
    std::shared_ptr<Tank> get_player_tank(size_t id);
    const std::shared_ptr<Tank> get_player_tank(size_t id) const;
    std::map<size_t, Player>& players();
    std::vector<std::vector<Cell>>& grid();
    bool execute_tank_action(std::shared_ptr<Tank> tank, TankAction action);
    void update();
    Position forward_position(const Position& pos, Direction dir) const;
    const Cell& get_cell(Position position) const;
    size_t get_height() const;
    size_t get_width() const;
    const std::string& input_file_name() const;

private:
    void update_shells();

    size_t width_, height_;
    std::string input_file_name_;
    std::vector<std::vector<Cell>> grid_;
    std::map<size_t, Player> players_;
};
