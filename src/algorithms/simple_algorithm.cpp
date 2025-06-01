#include "algorithms/simple_algorithm.h"
#include "algorithms/algorithm_utils.h"
#include "global_config.h"
#include <iostream>


SimpleAlgorithm::SimpleAlgorithm(int player_index, int tank_index)
    : AlgorithmBase(player_index, tank_index) {}

ActionRequest SimpleAlgorithm::getActionImpl() 
{
    // If a shell is coming, run away
    if (auto evade = getEvadeActionIfShellIncoming(std::max(width_, height_)))  // Be more conservative
    {
        if constexpr (config::get<bool>("verbose_debug"))
        {
            std::cout << "[SimpleAlgorithm] Evading a shell using: " << tankActionToString(*evade) << std::endl;
        }
        return *evade;
    }

    // Not under threat, check if we can shoot the opponent
    Position opponent_pos;
    if (tank_->canShoot() && hasLineOfSightToOpponent(tank_->position(), tank_->direction(), opponent_pos))
    {
        // He is just in front of us, shoot him!
        if constexpr (config::get<bool>("verbose_debug"))
        {
            std::cout << "[SimpleAlgorithm] Shooting opponent at " << opponent_pos 
                      << " from " << tank_->position() << std::endl;
        }
        return ActionRequest::Shoot;
    }

    // Always prefer requesting BattleInfo if we don't have something better to do
    if constexpr (config::get<bool>("verbose_debug"))
    {
        std::cout << "[SimpleAlgorithm] Nothing to do, requesting BattleInfo." << std::endl;
    }
    return ActionRequest::GetBattleInfo;
}
