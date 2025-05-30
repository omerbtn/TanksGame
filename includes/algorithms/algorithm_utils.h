#pragma once

#include "types/direction.h"
#include "types/position.h"
#include "ActionRequest.h"
#include "board.h"


// Represents a state in BFS with position and direction
struct BFSState
{
    Position pos;
    Direction dir;

    bool operator==(const BFSState& other) const
    {
        return pos == other.pos && dir == other.dir;
    }

    bool operator!=(const BFSState& other) const
    {
        return !(*this == other);
    }
};

// Hash specialization for BFSState to use in unordered containers
namespace std
{
    template <>
    struct hash<BFSState>
    {
        size_t operator()(const BFSState& state) const
        {
            return hash<Position>()(state.pos) ^ (hash<int>()(static_cast<int>(state.dir)) << 1);
        }
    };
}

const std::vector<Direction>& getAllDirections();

Direction getOppositeDirection(Direction dir);
Direction getDirectionAfterRotation(Direction dir, ActionRequest action);
std::string directionToString(Direction dir);
std::string directionToArrow(Direction dir);
std::string tank_action_to_string(ActionRequest action);
Direction getSeedDirection(int player_index);

Position forward_position(const Position& pos, Direction dir, size_t width, size_t height, size_t steps = 1);
Position backward_position(const Position& pos, Direction dir, size_t width, size_t height, size_t steps = 1);

size_t getNumberOfShellsInGrid(const std::vector<std::vector<Cell>>& grid);
bool isBlockedByWall(const std::vector<std::vector<Cell>>& grid, const Position& from, Direction dir, size_t steps);