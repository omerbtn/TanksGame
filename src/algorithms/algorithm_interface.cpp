#include "algorithms/algorithm_interface.h"
#include "algorithms/algorithm_utils.h"


// Returns an evasive action if there is an incoming shell toward the tank
// TODO: Change to evade the closest shell
std::optional<TankAction> AlgorithmInterface::getEvadeActionIfShellIncoming(const Tank &tank, const Board &board) 
{
    const Position& tankPos = tank.position();
    const Direction& tankDir = tank.direction();

    // Scan the board for shells
    for (int y = 0; y < board.getHeight(); ++y) 
    {
        for (int x = 0; x < board.getWidth(); ++x)
        {
            Position pos(x, y);
            const Cell& cell = board.getCell(pos);

            if (cell.has(ObjectType::Shell))  
            {
                auto shellDir = static_cast<Shell*>(cell.get_object(ObjectType::Shell).get())->direction();

                // Check if the shell has a line of sight to our tank
                if (hasLineOfSight(pos, tankPos, shellDir, board)) 
                {
                    // Shell is coming towards us, need to run away
                    // We don't want to run towards the shell, or at the opposite direction (because the shell is faster)
                    if (tankDir != getOppositeDirection(shellDir) &&
                        tankDir != shellDir) 
                    {
                        // We can just move forward and avoid the shell
                        // Before, we need to check if the next cell is safe
                        Position nextPos = board.forward_position(tankPos, tankDir);
                        const Cell& nextCell = board.getCell(nextPos);
                        
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
                            const Cell& newCell = board.getCell(newPos);

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
        }
    }

    return std::nullopt; // No incoming shell to evade
}