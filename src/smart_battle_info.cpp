#include "smart_battle_info.h"

#include "algorithms/algorithm_utils.h"

SmartBattleInfo::SmartBattleInfo(const SatelliteView& satellite_view, size_t height, size_t width,
                size_t max_steps, size_t num_shells, int player_index,
                const std::unordered_map<Position, std::unordered_set<Direction>>& shell_possible_directions,
                const std::unordered_map<int, std::unordered_set<Position>>& tanks_reserved_positions)
    : height_(height), width_(width), max_steps_(max_steps), num_shells_(num_shells),
    shell_possible_drections_(shell_possible_directions), tanks_reserved_positions_(tanks_reserved_positions), player_index_(player_index)
{
    grid_ = reconstructGridFromSatelliteView(satellite_view, height_, width_, player_index_, num_shells, tank_pos_);
}

const std::vector<std::vector<Cell>>& SmartBattleInfo::getGrid() const 
{ 
    return grid_; 
}

size_t SmartBattleInfo::getHeight() const 
{
    return height_; 
}

size_t SmartBattleInfo::getWidth() const 
{
     return width_; 
}

size_t SmartBattleInfo::getMaxSteps() const 
{ 
    return max_steps_;
}

size_t SmartBattleInfo::getNumShells() const 
{ 
    return num_shells_;
}

const Position& SmartBattleInfo::getTankPosition() const 
{
    return tank_pos_;
}

const std::unordered_map<Position, std::unordered_set<Direction>>& SmartBattleInfo::getShellPossibleDirections() const 
{ 
    return shell_possible_drections_; 
}

const std::unordered_map<int, std::unordered_set<Position>>& SmartBattleInfo::getTanksReservedPositions() const 
{ 
    return tanks_reserved_positions_;
}

void SmartBattleInfo::setTankReservedPositions(int tank_id, const std::unordered_set<Position>& reserved_positions)
{
    tanks_reserved_positions_[tank_id] = reserved_positions;
}

void SmartBattleInfo::setTanksReservedPositions(const std::unordered_map<int, std::unordered_set<Position>>& reserved_positions)
{
    tanks_reserved_positions_ = reserved_positions;
}