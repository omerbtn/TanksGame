#include "algorithms/algorithm_utils.h"

Direction getOppositeDirection(Direction dir)
{
    return static_cast<Direction>((static_cast<int>(dir) + 4) % 8);
}

Direction getDirectionAfterRotation(Direction dir, ActionRequest action)
{
    int offset = 0;

    switch (action)
    {
        case ActionRequest::RotateLeft45:
            offset = -1;
            break;
        case ActionRequest::RotateLeft90:
            offset = -2;
            break;
        case ActionRequest::RotateRight45:
            offset = 1;
            break;
        case ActionRequest::RotateRight90:
            offset = 2;
            break;
        default:
            break; // No rotation
    }

    return static_cast<Direction>((static_cast<int>(dir) + offset + 8) % 8);
}

std::string directionToString(Direction dir)
{
    switch (dir)
    {
        case Direction::U: return "U";
        case Direction::UR: return "UR";
        case Direction::R: return "R";
        case Direction::DR: return "DR";
        case Direction::D: return "D";
        case Direction::DL: return "DL";
        case Direction::L: return "L";
        case Direction::UL: return "UL";
        default: return "Unknown direction";
    }
}

std::string tankActionToString(ActionRequest action)
{
    switch (action)
    {
        case ActionRequest::MoveForward: return "MoveForward";
        case ActionRequest::MoveBackward: return "MoveBackward";
        case ActionRequest::RotateLeft90: return "RotateLeft90";
        case ActionRequest::RotateRight90: return "RotateRight90";
        case ActionRequest::RotateLeft45: return "RotateLeft45";
        case ActionRequest::RotateRight45: return "RotateRight45";
        case ActionRequest::Shoot: return "Shoot";
        case ActionRequest::GetBattleInfo: return "GetBattleInfo";
        case ActionRequest::DoNothing: return "DoNothing";
        default: return "Unknown Action";
    }
}

std::string directionToArrow(Direction dir)
{
    switch (dir)
    {
        case Direction::U: return "↑";
        case Direction::UR: return "↗";
        case Direction::R: return "→";
        case Direction::DR: return "↘";
        case Direction::D: return "↓";
        case Direction::DL: return "↙";
        case Direction::L: return "←";
        case Direction::UL: return "↖";
        default: return "?"; // Unknown direction
    }
}

Position forwardPosition(const Position& pos, Direction dir, size_t width, size_t height, size_t steps) 
{
    static const std::unordered_map<Direction, std::pair<int, int>> deltas = {
        {Direction::U, {0, -1}}, {Direction::UR, {1, -1}}, {Direction::R, {1, 0}},  {Direction::DR, {1, 1}},
        {Direction::D, {0, 1}},  {Direction::DL, {-1, 1}}, {Direction::L, {-1, 0}}, {Direction::UL, {-1, -1}}};

    auto [dx, dy] = deltas.at(dir);
    int new_x = (pos.first + (dx + width) * steps) % width;
    int new_y = (pos.second + (dy + height) * steps) % height;

    return Position(new_x, new_y);
}

Position backwardPosition(const Position& pos, Direction dir, size_t width, size_t height, size_t steps) 
{
    return forwardPosition(pos, getOppositeDirection(dir), width, height, steps);
}

Direction getSeedDirection(int player_index)
{
    return (player_index % 2 == 1) ? Direction::L : Direction::R;
}

size_t getNumberOfShellsInGrid(const std::vector<std::vector<Cell>>& grid)
{
    size_t count = 0;
    for (const auto& row : grid)
    {
        for (const auto& cell : row)
        {
            if (cell.has(ObjectType::Shell))
            {
                ++count;
            }
        }
    }
    return count;
}

// Moving *backward* from a position in a given direction, checking if there are walls blocking the path
bool isBlockedByWall(const std::vector<std::vector<Cell>>& grid, const Position& from, Direction dir, size_t steps)
{
    if (grid.empty() || grid[0].empty()) {
        return true;  // If grid is empty, assume walls are blocking
    }

    size_t width = grid[0].size();
    size_t height = grid.size();
    Position pos = from;
    for (size_t i = 0; i < steps; ++i)
    {
        pos = backwardPosition(pos, dir, width, height);
        if (grid[pos.first][pos.second].has(ObjectType::Wall))
        {
            return true;
        }
    }
    return false;
}

const std::vector<Direction>& getAllDirections()
{
    static const std::vector<Direction> all_directions = {
        Direction::U, Direction::UR, Direction::R, Direction::DR,
        Direction::D, Direction::DL, Direction::L, Direction::UL
    };
    return all_directions;
}