#pragma once

#include <map>
#include <unordered_set>
#include <vector>

#include "ActionRequest.h"
#include "Player.h"
// #include "PlayerFactory.h"
// #include "TankAlgorithmFactory.h"

#include "cell.h"
#include "game_info.h"
#include "tank.h"
#include "types/direction.h"
#include "types/position.h"


class Board
{
public:
    Board();

    Board(const Board&) = delete;
    Board& operator=(const Board&) = delete;
    Board(Board&&) = delete;
    Board& operator=(Board&&) = delete;

    GameInfo loadFromSatelliteView(size_t map_width, size_t map_height,
                                   const SatelliteView& map,
                                   size_t max_steps, size_t num_shells,
                                   Player& player1, Player& player2,
                                   TankAlgorithmFactory player1_tank_algo_factory,
                                   TankAlgorithmFactory player2_tank_algo_factory);

    void print() const;

    const std::shared_ptr<Tank> getTank(int player_id, int tank_id) const;
    const std::vector<std::shared_ptr<Tank>>& getPlayerTanks(int player_id) const;

    const Cell& getCell(Position position) const;
    size_t getHeight() const;
    size_t getWidth() const;
    std::vector<std::vector<Cell>>& grid();
    const std::vector<std::vector<Cell>>& getGrid() const;
    bool executeTankAction(std::shared_ptr<Tank> tank, ActionRequest& action);
    void doShellsStep(bool shells_only = true);
    void update();
    TankAlgorithm* getAlgorithm(int player_id, int tank_id);

private:
    void updateActiveShells();
    void resolveCollisions(Cell& cell);
    void onExplosion(Cell& cell);
    bool moveTankBackward(std::shared_ptr<Tank> tank, const Position& current_pos);
    bool rotateTank(std::shared_ptr<Tank> tank, ActionRequest action);
    bool shoot(std::shared_ptr<Tank> tank, const Position& current_pos);
    bool getBattleInfo(std::shared_ptr<Tank> tank);
    bool doNothing(std::shared_ptr<Tank> tank);
    bool moveTankForward(std::shared_ptr<Tank> tank, const Position& current_pos);
    bool handleBackMovement(std::shared_ptr<Tank> tank, const Position& current_pos);

    size_t width_, height_;
    std::vector<std::vector<Cell>> grid_;
    std::vector<std::vector<Cell>> prev_grid_; // Previous state of the grid, used for GetBattleInfo
    std::vector<std::pair<Position, std::shared_ptr<Shell>>> active_shells_;
    std::unordered_set<Position> cells_to_update_;
    std::unordered_map<Position, std::shared_ptr<Tank>> old_tanks_positions_;
    std::map<std::pair<size_t, size_t>, std::unique_ptr<TankAlgorithm>> algorithms_;
    std::map<int, std::pair<std::reference_wrapper<Player>, std::vector<std::shared_ptr<Tank>>>> player_tanks_;
};
