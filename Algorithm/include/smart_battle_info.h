#pragma once

#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include "BattleInfo.h"
#include "SatelliteView.h"
#include "cell.h"
#include "types/direction.h"


namespace Algorithm_322573304_322647603
{

// Bring necessary types from UserCommon
using UserCommon_322573304_322647603::Direction;
using UserCommon_322573304_322647603::Position;

class SmartBattleInfo : public BattleInfo
{
public:
    virtual ~SmartBattleInfo() = default;
    SmartBattleInfo(const SatelliteView& satellite_view, size_t height, size_t width,
                    size_t max_steps, size_t num_shells,
                    const std::unordered_map<Position, std::unordered_set<Direction>>& shell_possible_directions = {},
                    size_t shell_pos_offset = 0)
        : satellite_view_(satellite_view), height_(height), width_(width),
          max_steps_(max_steps), num_shells_(num_shells),
          shell_possible_directions_(shell_possible_directions),
          shell_pos_offset_(shell_pos_offset) {}

    SmartBattleInfo(const SmartBattleInfo&) = delete;
    SmartBattleInfo& operator=(const SmartBattleInfo&) = delete;
    SmartBattleInfo(SmartBattleInfo&&) = delete;
    SmartBattleInfo& operator=(SmartBattleInfo&&) = delete;

    const SatelliteView& getSatelliteView() const { return satellite_view_; }
    size_t getHeight() const { return height_; }
    size_t getWidth() const { return width_; }
    size_t getMaxSteps() const { return max_steps_; }
    size_t getNumShells() const { return num_shells_; }
    const std::unordered_map<Position, std::unordered_set<Direction>>& getShellPossibleDirections() const { return shell_possible_directions_; }
    const std::unordered_map<int, std::unordered_set<Position>>& getTanksReservedPositions() const { return tanks_reserved_positions_; }
    const std::unordered_map<Position, size_t>& getWallsDamage() const { return walls_damage_; }
    size_t getShellPosOffset() const { return shell_pos_offset_; }

    void setTankReservedPositions(int tank_id, const std::unordered_set<Position>& reserved_positions) // To be used by the tanks
    {
        tanks_reserved_positions_[tank_id] = reserved_positions;
    }
    void setTanksReservedPositions(const std::unordered_map<int, std::unordered_set<Position>>& reserved_positions) // To be used by the player
    {
        tanks_reserved_positions_ = reserved_positions;
    }

    // 'clear_previous = true' used by the player, 'clear_previous = false' used by the tanks to accumulate damage
    void setWallsDamage(const std::unordered_map<Position, size_t>& walls_damage, bool clear_previous = false)
    {
        if (clear_previous)
        {
            walls_damage_.clear();
        }
        for (const auto& [pos, damage] : walls_damage)
        {
            walls_damage_[pos] += damage; // Accumulate damage
        }
    }

    void reportShellDirection(const Position& pos, Direction direction)
    {
        // Validate the position really contains a shell
        if (satellite_view_.getObjectAt(pos.first, pos.second) == '*')
        {
            shell_possible_directions_[pos] = {direction};
        }
    }
    void setShellPosOffset(size_t offset)
    {
        shell_pos_offset_ = offset;
    }

private:
    const SatelliteView& satellite_view_;
    size_t height_;
    size_t width_;
    size_t max_steps_;
    size_t num_shells_;
    std::unordered_map<Position, std::unordered_set<Direction>> shell_possible_directions_;
    std::unordered_map<int, std::unordered_set<Position>> tanks_reserved_positions_;
    std::unordered_map<Position, size_t> walls_damage_; // Wall's position -> number of hits it has taken
    size_t shell_pos_offset_ = 0;
};

} // namespace Algorithm_322573304_322647603