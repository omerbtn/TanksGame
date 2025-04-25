#include "algorithms/algorithm_utils.h"

Direction getDirectionTo(int sx, int sy, int tx, int ty)
{
    int dx = tx - sx;
    int dy = sy - ty;

    if (dx == 0 && dy > 0)
        return Direction::U;
    if (dx > 0 && dy > 0)
        return Direction::UR;
    if (dx > 0 && dy == 0)
        return Direction::R;
    if (dx > 0 && dy < 0)
        return Direction::DR;
    if (dx == 0 && dy < 0)
        return Direction::D;
    if (dx < 0 && dy < 0)
        return Direction::DL;
    if (dx < 0 && dy == 0)
        return Direction::L;
    if (dx < 0 && dy > 0)
        return Direction::UL;

    return Direction::U; // fallback
}

bool hasLineOfSight(const Position& from, const Position& to, Direction dir, const Board& board)
{
    Position current = board.forward_position(from, dir);

    //while (current != from)
    for (int steps = 0; steps < board.getWidth(); ++steps)
    {
        if (current == to)
            return true;

        const Cell& cell = board.getCell(current);
        if (cell.has(ObjectType::Wall))
            return false;
            
        current = board.forward_position(current, dir);
    }

    return false;
}

Direction getDirectionTo(const Position& from, const Position& to)
{
    return getDirectionTo(from.first, from.second, to.first, to.second);
}

Direction getOppositeDirection(Direction dir)
{
    return static_cast<Direction>((static_cast<int>(dir) + 4) % 8);
}

Direction getDirectionAfterRotation(Direction dir, TankAction action)
{
    int offset = 0;
    
    switch (action)
    {
        case TankAction::RotateLeft_1_8:
            offset = -1;
            break;
        case TankAction::RotateLeft_1_4:
            offset = -2;
            break;
        case TankAction::RotateRight_1_8:
            offset = 1;
            break;
        case TankAction::RotateRight_1_4:
            offset = 2;
            break;
        default:
            return dir; // No rotation
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

std::string tankActionToString(TankAction action)
{
    switch (action)
    {
        case TankAction::MoveForward: return "MoveForward";
        case TankAction::MoveBackward: return "MoveBackward";
        case TankAction::RotateLeft_1_8: return "RotateLeft_1_8";
        case TankAction::RotateRight_1_8: return "RotateRight_1_8";
        case TankAction::RotateLeft_1_4: return "RotateLeft_1_4";
        case TankAction::RotateRight_1_4: return "RotateRight_1_4";
        case TankAction::Shoot: return "Shoot";
        case TankAction::Idle: return "Idle";
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