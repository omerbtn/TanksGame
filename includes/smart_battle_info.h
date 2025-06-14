#pragma once

#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include "BattleInfo.h"
#include "SatelliteView.h"
#include "cell.h"


class SmartBattleInfo : public BattleInfo
{
public:
    ~SmartBattleInfo() = default;
    SmartBattleInfo(const SatelliteView& satellite_view, size_t height, size_t width,
                    size_t max_steps, size_t num_shells, int player_index,
                    const std::unordered_map<Position, std::unordered_set<Direction>>& shell_possible_directions = {},
                    const std::unordered_map<int, std::unordered_set<Position>>& tanks_reserved_positions = {});

    SmartBattleInfo(const SmartBattleInfo&) = delete;
    SmartBattleInfo& operator=(const SmartBattleInfo&) = delete;
    SmartBattleInfo(SmartBattleInfo&&) = delete;
    SmartBattleInfo& operator=(SmartBattleInfo&&) = delete;

    const std::vector<std::vector<Cell>>& getGrid() const;
    size_t getHeight() const;
    size_t getWidth() const;
    size_t getMaxSteps() const;
    size_t getNumShells() const;
    const Position& getTankPosition() const;
    const std::unordered_map<Position, std::unordered_set<Direction>>& getShellPossibleDirections() const;
    const std::unordered_map<int, std::unordered_set<Position>>& getTanksReservedPositions() const;

    void setTankReservedPositions(int tank_id, const std::unordered_set<Position>& reserved_positions); // To be used by the tanks
    void setTanksReservedPositions(const std::unordered_map<int, std::unordered_set<Position>>& reserved_positions);

private:
    std::vector<std::vector<Cell>> grid_;
    size_t height_;
    size_t width_;
    size_t max_steps_;
    size_t num_shells_;
    Position tank_pos_;
    std::unordered_map<Position, std::unordered_set<Direction>> shell_possible_drections_;
    std::unordered_map<int, std::unordered_set<Position>> tanks_reserved_positions_;
    int player_index_;
};