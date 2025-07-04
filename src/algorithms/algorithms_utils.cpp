#include "algorithms/algorithm_utils.h"

#include "global_config.h"
#include "printers/ansi_printer.h"
#include "printers/default_printer.h"


void printGrid(const std::vector<std::vector<Cell>>& grid)
{
    using SelectedPrinter = std::conditional_t<config::get<bool>("use_ansi_printer"), AnsiPrinter, DefaultPrinter>;
    SelectedPrinter printer(grid);
    printer.print();
}

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
    case Direction::U:
        return "U";
    case Direction::UR:
        return "UR";
    case Direction::R:
        return "R";
    case Direction::DR:
        return "DR";
    case Direction::D:
        return "D";
    case Direction::DL:
        return "DL";
    case Direction::L:
        return "L";
    case Direction::UL:
        return "UL";
    default:
        return "Unknown direction";
    }
}

std::string tankActionToString(ActionRequest action)
{
    switch (action)
    {
    case ActionRequest::MoveForward:
        return "MoveForward";
    case ActionRequest::MoveBackward:
        return "MoveBackward";
    case ActionRequest::RotateLeft90:
        return "RotateLeft90";
    case ActionRequest::RotateRight90:
        return "RotateRight90";
    case ActionRequest::RotateLeft45:
        return "RotateLeft45";
    case ActionRequest::RotateRight45:
        return "RotateRight45";
    case ActionRequest::Shoot:
        return "Shoot";
    case ActionRequest::GetBattleInfo:
        return "GetBattleInfo";
    case ActionRequest::DoNothing:
        return "DoNothing";
    default:
        return "Unknown Action";
    }
}

std::string directionToArrow(Direction dir)
{
    switch (dir)
    {
    case Direction::U:
        return "↑";
    case Direction::UR:
        return "↗";
    case Direction::R:
        return "→";
    case Direction::DR:
        return "↘";
    case Direction::D:
        return "↓";
    case Direction::DL:
        return "↙";
    case Direction::L:
        return "←";
    case Direction::UL:
        return "↖";
    default:
        return "?"; // Unknown direction
    }
}

Position forwardPosition(const Position& pos, Direction dir, size_t width, size_t height, size_t steps)
{
    static const std::unordered_map<Direction, std::pair<int, int>> deltas = {
        {Direction::U, {0, -1}}, {Direction::UR, {1, -1}}, {Direction::R, {1, 0}}, {Direction::DR, {1, 1}},
        {Direction::D, {0, 1}}, {Direction::DL, {-1, 1}}, {Direction::L, {-1, 0}}, {Direction::UL, {-1, -1}}
    };

    auto [dx, dy] = deltas.at(dir);
    int new_x = (pos.first + (dx + width) * steps) % width;
    int new_y = (pos.second + (dy + height) * steps) % height;

    return Position(new_x, new_y);
}

Position backwardPosition(const Position& pos, Direction dir, size_t width, size_t height, size_t steps)
{
    return forwardPosition(pos, getOppositeDirection(dir), width, height, steps);
}

size_t getDistance(const Position& from, const Position& to, Direction dir, size_t width, size_t height)
{
    Position current = from;
    size_t distance = 0;

    while (current != to)
    {
        current = forwardPosition(current, dir, width, height);
        ++distance;
    }

    return distance;
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
    if (grid.empty() || grid[0].empty())
    {
        return true; // If grid is empty, assume walls are blocking
    }

    size_t height = grid.size();
    size_t width = grid[0].size();
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

std::vector<std::vector<Cell>> reconstructGridFromSatelliteView(const SatelliteView& satellite_view, size_t height,
                                                                size_t width, int player_index, size_t num_shells,
                                                                Position& r_tank_pos)
{
    std::vector<std::vector<Cell>> grid(height, std::vector<Cell>(width));

    for (size_t y = 0; y < height; ++y)
    {
        for (size_t x = 0; x < width; ++x)
        {
            char ch = satellite_view.getObjectAt(x, y);
            Position pos{x, y};
            Cell cell(pos);

            switch (ch)
            {
            case '#':
                cell.addObject(std::make_shared<Wall>());
                break;
            case '@':
                cell.addObject(std::make_shared<Mine>());
                break;
            case '*':
                cell.addObject(std::make_shared<Shell>(Direction::U)); // Direction is unreliable
                break;
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                cell.addObject(std::make_shared<Tank>(ch - '0', 0, pos, getSeedDirection(ch - '0'),
                                                      num_shells)); // Direction is unreliable unless it's first turn
                break;
            case '%':
                cell.addObject(std::make_shared<Tank>(player_index, 0, pos, getSeedDirection(player_index),
                                                      num_shells)); // Direction is unreliable unless it's first turn
                r_tank_pos = pos;
                break;
            default:
                break;
            }

            grid[x][y] = std::move(cell);
        }
    }

    return grid;
}