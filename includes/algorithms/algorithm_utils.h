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

bool hasLineOfSight(const Position& from, const Position& to, Direction dir, const std::vector<std::vector<Cell>>& grid);
Direction getOppositeDirection(Direction dir);
Direction getDirectionAfterRotation(Direction dir, ActionRequest action);
std::string directionToString(Direction dir);
std::string directionToArrow(Direction dir);
std::string tank_action_to_string(ActionRequest action);

std::shared_ptr<Tank> getOpponent(const int player_index, const std::vector<std::vector<Cell>>& grid);
Position forward_position(const Position& pos, Direction dir, const size_t width, const size_t height);
Direction getSeedDirection(int player_index);
