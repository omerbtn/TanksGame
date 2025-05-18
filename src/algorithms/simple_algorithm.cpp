#include "algorithms/simple_algorithm.h"
#include "algorithms/algorithm_utils.h"

ActionRequest SimpleAlgorithm::getActionImpl() {
    const Position& tank_pos = tank_->position();
    const Direction& tank_dir = tank_->direction();

    // If a shell is coming, run away
    /*if (auto evade = getEvadeActionIfShellIncoming(tank, board, std::max(board.get_width(), board.get_height())))  // Be more conservative
    {
        return *evade;
    }*/

    // Not under threat - can we shoot the opponent?

    auto opponent = getOpponent(tank_->id(), grid_);
    if (opponent && opponent->is_alive() && hasLineOfSight(tank_pos, opponent->position(), tank_dir, grid_)) {
        // He is just in front of us, shoot him!
        return ActionRequest::Shoot;
    }

    // Otherwise, just do nothing
    return ActionRequest::DoNothing;
}

/*TankAction SimpleAlgorithm::decideAction(const Tank &tank, const Board &board)
{
    const Position& tank_pos = tank.position();
    const Direction& tank_dir = tank.direction();

    // If a shell is coming, run away
    if (auto evade = getEvadeActionIfShellIncoming(tank, board, std::max(board.get_width(), board.get_height())))  // Be more conservative
    {
        return *evade;
    }

    // Not under threat - can we shoot the opponent?
    const std::shared_ptr<Tank> opponent = board.get_player_tank(tank.id() == 1 ? 2 : 1);
    if (opponent && opponent->is_alive() && hasLineOfSight(tank_pos, opponent->position(), tank_dir, board))
    {
        // He is just in front of us, shoot him!
        return TankAction::Shoot;
    }

    // Otherwise, just do nothing
    return TankAction::Idle;
}*/
