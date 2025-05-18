#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_set>
#include <optional>

#include "position.h"
#include "direction.h"
#include "ActionRequest.h"
#include "TankAlgorithmFactory.h"
#include "PlayerFactory.h"

#include "cell.h"
#include "Player.h"
#include "game_info.h"
#include "tank.h"


class Board
{
public:
    Board(const PlayerFactory& playerFactory, const TankAlgorithmFactory& algorithmFactory);

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
    TankAlgorithm* get_algorithm(int player_id, int tank_id);

private:
    void update_active_shells();
    void resolve_collisions(Cell& cell);
    void on_explosion(Cell& cell);

    const PlayerFactory& playerFactory_;
    const TankAlgorithmFactory& algorithmFactory_;

    size_t width_, height_;
    std::vector<std::vector<Cell>> grid_;
    std::vector<std::pair<Position, std::shared_ptr<Shell>>> active_shells_;
    std::unordered_set<Position> cells_to_update_;
    std::unordered_map<Position, std::shared_ptr<Tank>> old_tanks_positions_;
    std::map<std::pair<size_t, size_t>, std::unique_ptr<TankAlgorithm>> algorithms_;
    std::map<int, std::pair<std::unique_ptr<Player>, std::vector<std::shared_ptr<Tank>>>> player_tanks_;
};
