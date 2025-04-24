#pragma once

#include <string>
#include <vector>
#include <map>

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
    Tank* get_player_tank(size_t id);
    const Tank* get_player_tank(size_t id) const;
    std::map<size_t, Player>& players();
    bool execute_tank_action(Tank* tank, TankAction action);
    void update_shells();
    void update();
    Position forward_position(const Position& pos, Direction dir) const;

private:
    size_t width_, height_;
    std::vector<std::vector<Cell>> grid_;
    std::map<size_t, Player> players_;
};
