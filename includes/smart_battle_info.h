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
                    size_t max_steps, size_t num_shells,
                    const std::unordered_map<Position, std::unordered_set<Direction>>& shell_possible_directions = {},
                    const std::unordered_map<int, std::unordered_set<Position>>& tanks_reserved_positions = {})
        : satellite_view_(satellite_view), height_(height), width_(width),
          max_steps_(max_steps), num_shells_(num_shells),
          shell_possible_drections_(shell_possible_directions),
          tanks_reserved_positions_(tanks_reserved_positions) {}

    SmartBattleInfo(const SmartBattleInfo&) = delete;
    SmartBattleInfo& operator=(const SmartBattleInfo&) = delete;
    SmartBattleInfo(SmartBattleInfo&&) = delete;
    SmartBattleInfo& operator=(SmartBattleInfo&&) = delete;

    const SatelliteView& getSatelliteView() const { return satellite_view_; }
    size_t getHeight() const { return height_; }
    size_t getWidth() const { return width_; }
    size_t getMaxSteps() const { return max_steps_; }
    size_t getNumShells() const { return num_shells_; }
    const std::unordered_map<Position, std::unordered_set<Direction>>& getShellPossibleDirections() const { return shell_possible_drections_; }
    const std::unordered_map<int, std::unordered_set<Position>>& getTanksReservedPositions() const { return tanks_reserved_positions_; }

    void setTankReservedPositions(int tank_id, const std::unordered_set<Position>& reserved_positions) // To be used by the tanks
    {
        tanks_reserved_positions_[tank_id] = reserved_positions;
    }
    void setTanksReservedPositions(const std::unordered_map<int, std::unordered_set<Position>>& reserved_positions) // To be used by the player
    {
        tanks_reserved_positions_ = reserved_positions;
    }

private:
    const SatelliteView& satellite_view_;
    size_t height_;
    size_t width_;
    size_t max_steps_;
    size_t num_shells_;
    std::unordered_map<Position, std::unordered_set<Direction>> shell_possible_drections_;
    std::unordered_map<int, std::unordered_set<Position>> tanks_reserved_positions_;
};