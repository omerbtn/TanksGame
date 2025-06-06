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

    GameInfo loadFromFile(const std::string& filename);
    void print() const;

    const std::shared_ptr<Tank> getTank(int player_id, int tank_id) const;
    const std::vector<std::shared_ptr<Tank>>& getPlayerTanks(int player_id) const;

    const Cell& getCell(Position position) const;
    size_t getHeight() const;
    size_t getWidth() const;
    std::vector<std::vector<Cell>>& grid();
    const std::vector<std::vector<Cell>>& getGrid() const;
    bool executeTankAction(std::shared_ptr<Tank> tank, ActionRequest action);
    void doShellsStep(bool shells_only = true);
    void update();
    TankAlgorithm* getAlgorithm(int player_id, int tank_id);

private:
    void updateActiveShells();
    void resolveCollisions(Cell& cell);
    void onExplosion(Cell& cell);

    const PlayerFactory& playerFactory_;
    const TankAlgorithmFactory& algorithmFactory_;

    size_t width_, height_;
    std::vector<std::vector<Cell>> grid_;
    std::vector<std::vector<Cell>> prev_grid_;  // Previous state of the grid, used for GetBattleInfo
    std::vector<std::pair<Position, std::shared_ptr<Shell>>> active_shells_;
    std::unordered_set<Position> cells_to_update_;
    std::unordered_map<Position, std::shared_ptr<Tank>> old_tanks_positions_;
    std::map<std::pair<size_t, size_t>, std::unique_ptr<TankAlgorithm>> algorithms_;
    std::map<int, std::pair<std::unique_ptr<Player>, std::vector<std::shared_ptr<Tank>>>> player_tanks_;
};
