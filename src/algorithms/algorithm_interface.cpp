#include "algorithms/algorithm_interface.h"
#include "algorithms/algorithm_utils.h"


// Returns an evasive action if there is an incoming shell toward the tank
// TODO: Change to evade the closest shell
std::optional<TankAction> AlgorithmInterface::getEvadeActionIfShellIncoming(const Tank &tank, const Board &board, size_t shell_max_distance)
{
    const Position& tankPos = tank.position();
    const Direction& tankDir = tank.direction();

    std::vector<std::pair<Position, Direction>> to_check = {
        {tankPos, Direction::U}, {tankPos, Direction::UR},
        {tankPos, Direction::R}, {tankPos, Direction::DR},
        {tankPos, Direction::D}, {tankPos, Direction::DL},
        {tankPos, Direction::L}, {tankPos, Direction::UL}
    };

    for (size_t i = 0; i < shell_max_distance; ++i)
    {
        std::vector<std::pair<Position, Direction>> next_to_check;

        for (auto& [pos, dir] : to_check)
        {
            pos = board.forward_position(pos, dir);
            const Cell& cell = board.get_cell(pos);

            if (cell.has(ObjectType::Wall))
            {
                // We hit a wall, it'll protect us, no shell from this direction
                continue;
            }

            if (cell.has(ObjectType::Shell))
            {
                auto shell = static_pointer_cast<Shell>(cell.get_object_by_type(ObjectType::Shell));
                if (!shell) continue; // Safety check

                Direction shellDir = shell->direction();

                // If we made it through, the shell has a line of sight to us
                if (shellDir == getOppositeDirection(dir))
                {
                    // Shell is coming towards us, need to run away
                    // We don't want to run towards the shell, or at the opposite direction (because the shell is faster)
                    if (tankDir != getOppositeDirection(shellDir) &&
                        tankDir != shellDir)
                    {
                        // We can just move forward and evade the shell
                        // Before, we need to check if the next cell is safe
                        Position nextPos = board.forward_position(tankPos, tankDir);
                        const Cell& nextCell = board.get_cell(nextPos);

                        if (nextCell.empty())
                        {
                            return TankAction::MoveForward;
                        }
                    }

                    // Else, we need to rotate away from the shell to a safe direction
                    std::vector<TankAction> rotationOptions = {
                        TankAction::RotateLeft_1_4, TankAction::RotateRight_1_4,
                        TankAction::RotateLeft_1_8, TankAction::RotateRight_1_8
                    };

                    for (auto rotateAction : rotationOptions)
                    {
                        Direction newDir = getDirectionAfterRotation(tankDir, rotateAction);
                        // Avoid facing toward the shell or the opposite direction
                        if (newDir != shellDir && newDir != getOppositeDirection(shellDir))
                        {
                            Position newPos = board.forward_position(tankPos, newDir);
                            const Cell& newCell = board.get_cell(newPos);

                            if (newCell.empty())
                            {
                                return rotateAction; // Rotate away from the shell, so next turn we can move forward
                            }
                        }
                    }

                    // If we didn't find a safe rotation, try to rotate right and try again next turn
                    return TankAction::RotateRight_1_4;
                }
            }

            // Continue scanning in this direction
            next_to_check.push_back({pos, dir});
        }

        // Update directions for the next distance layer
        to_check = next_to_check;
    }

    return std::nullopt; // No incoming shell to evade
}
