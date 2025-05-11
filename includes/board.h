#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_set>
#include <optional>

#include "position.h"
#include "direction.h"
#include "ActionRequest.h"

#include "cell.h"
#include "player.h"
#include "game_info.h"
#include "tank.h"


class Board
{
public:
    Board() = default;

    GameInfo load_from_file(const std::string& filename);
    void print() const;

    const std::shared_ptr<Tank> get_tank(int player_id, int tank_id) const;
    const std::vector<std::shared_ptr<Tank>>& get_player_tanks(int player_id) const;

    const Cell& get_cell(Position position) const;
    size_t get_height() const;
    size_t get_width() const;
    std::vector<std::vector<Cell>>& grid();
    bool execute_tank_action(std::shared_ptr<Tank> tank, ActionRequest action);
    void do_shells_step();
    void update();
    
    Position forward_position(const Position& pos, Direction dir) const;
    
    /*std::shared_ptr<Tank> get_player_tank(size_t id);
    const std::shared_ptr<Tank> get_player_tank(size_t id) const;
    std::map<size_t, Player>& players();
    const std::string& input_file_name() const;
    */


private:
    void update_active_shells();
    void resolve_collisions(Cell& cell);
    void on_explosion(Cell& cell);

    size_t width_, height_;
    //std::string input_file_name_;
    std::vector<std::vector<Cell>> grid_;
    std::vector<std::shared_ptr<Tank>> player1_tanks_;  // To be able to get a tank by player id and tank id.
    std::vector<std::shared_ptr<Tank>> player2_tanks_;
    std::vector<std::pair<Position, std::shared_ptr<Shell>>> active_shells_;
    std::unordered_set<Position> cells_to_update_;
    std::unordered_map<Position, std::shared_ptr<Tank>> old_tanks_positions_;
    /*std::map<size_t, Player> players_;
    */
};
