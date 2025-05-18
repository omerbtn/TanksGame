#include "algorithms/algorithm_utils.h"

bool hasLineOfSight(const Position& from, const Position& to, Direction dir, const std::vector<std::vector<Cell>>& grid) {
    auto width = grid[0].size();
    auto height = grid.size();

    Position current = forward_position(from, dir, width, height);

    for (size_t steps = 0; steps < std::max(width, height); ++steps) {
        if (current == to)
            return true;

        if (current == from)
            return false; // We are back to the starting position

        int x = current.first % width;
        int y = current.second % height;

        const Cell& cell = grid[x][y];
        if (cell.has(ObjectType::Wall))
            return false;

        current = forward_position(current, dir, width, height);
    }

    return false;
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

std::string tank_action_to_string(ActionRequest action)
{
    switch (action)
    {
        case ActionRequest::MoveForward: return "MoveForward";
        case ActionRequest::MoveBackward: return "MoveBackward";
        case ActionRequest::RotateLeft90: return "RotateLeft_1_4";
        case ActionRequest::RotateRight90: return "RotateRight_1_4";
        case ActionRequest::RotateLeft45: return "RotateLeft_1_8";
        case ActionRequest::RotateRight45: return "RotateRight_1_8";
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

std::shared_ptr<Tank> getOpponent(const int player_index, const std::vector<std::vector<Cell>>& grid) {
    for (size_t x = 0; x < grid.size(); ++x)
    {
        for (size_t y = 0; y < grid[x].size(); ++y)
        {
            const Cell& cell = grid[x][y];
            if (cell.has(ObjectType::Tank)) {
                auto tank = std::static_pointer_cast<Tank>(cell.get_object_by_type(ObjectType::Tank));
                if (tank->player_id() != player_index)
                {
                    return tank;
                }
            }
        }
    }

    return nullptr; // Not found
}

Position forward_position(const Position& pos, Direction dir, const size_t width, const size_t height) {
    static const std::unordered_map<Direction, std::pair<int, int>> deltas = {
        {Direction::U, {0, -1}}, {Direction::UR, {1, -1}}, {Direction::R, {1, 0}},  {Direction::DR, {1, 1}},
        {Direction::D, {0, 1}},  {Direction::DL, {-1, 1}}, {Direction::L, {-1, 0}}, {Direction::UL, {-1, -1}}};
    auto [dx, dy] = deltas.at(dir);
    int new_x = (pos.first + dx + width) % width;
    int new_y = (pos.second + dy + height) % height;

    return Position(new_x, new_y);
}

Direction getSeedDirection(int player_index)
{
    return (player_index % 2 == 1) ? Direction::L : Direction::R;
}
