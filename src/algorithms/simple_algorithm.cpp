#include "algorithms/simple_algorithm.h"
#include "algorithms/algorithm_utils.h"


TankAction SimpleAlgorithm::decideAction(const Tank &tank, const Board &board) 
{
    const Position& tankPos = tank.position();
    const Direction& tankDir = tank.direction();

    // If a shell is coming, run away
    if (auto evade = getEvadeActionIfShellIncoming(tank, board, std::max(board.get_width(), board.get_height())))  // Be more conservative
    {
        return *evade;
    }

    // Not under threat - can we shoot the opponent?
    const std::shared_ptr<Tank> opponent = board.get_player_tank(tank.id() == 1 ? 2 : 1);
    if (opponent && opponent->is_alive() && hasLineOfSight(tankPos, opponent->position(), tankDir, board)) 
    {
        // He is just in front of us, shoot him!
        return TankAction::Shoot;
    }

    // Otherwise, just do nothing
    return TankAction::Idle;
}