#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_set>
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

    /*std::shared_ptr<Tank> get_player_tank(size_t id);
    const std::shared_ptr<Tank> get_player_tank(size_t id) const;
    const Cell& get_cell(Position position) const;
    size_t get_height() const;
    size_t get_width() const;
    std::map<size_t, Player>& players();
    std::vector<std::vector<Cell>>& grid();
    const std::string& input_file_name() const;

    bool execute_tank_action(std::shared_ptr<Tank> tank, TankAction action);
    void do_shells_step();
    void update();

    Position forward_position(const Position& pos, Direction dir) const;


private:
    void update_active_shells();
    void resolve_collisions(Cell& cell);
    void on_explosion(Cell& cell);*/

    size_t width_, height_;
    std::string input_file_name_;
    std::vector<std::vector<Cell>> grid_;
    /*std::map<size_t, Player> players_;
    std::vector<std::pair<Position, std::shared_ptr<Shell>>> active_shells_;
    std::unordered_set<Position> cells_to_update_;
    std::unordered_map<Position, std::shared_ptr<Tank>> old_tanks_positions_;*/
};
