#pragma once

#include "types/direction.h"
#include "types/position.h"
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

Direction getDirectionTo(int sx, int sy, int tx, int ty);
bool hasLineOfSight(const Position& from, const Position& to, Direction dir, const Board& board);
Direction getDirectionTo(const Position& from, const Position& to);
Direction getOppositeDirection(Direction dir);
Direction getDirectionAfterRotation(Direction dir, TankAction action);
std::string directionToString(Direction dir);
std::string directionToArrow(Direction dir);
std::string tank_action_to_string(TankAction action);
