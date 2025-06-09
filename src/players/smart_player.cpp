#include "smart_player.h"
#include "algorithm_utils.h"
#include "smart_battle_info.h"


SmartPlayer::SmartPlayer(int player_index, size_t x, size_t y, size_t max_steps, size_t num_shells)
    : PlayerBase(player_index, x, y, max_steps, num_shells) {}

void SmartPlayer::updateTankWithBattleInfo(TankAlgorithm& tank, SatelliteView& satellite_view)
{
    SmartBattleInfo info = createBattleInfo(satellite_view);

    // Extend the battle info with reserved positions
    info.setTanksReservedPositions(tanks_reserved_positions_);

    tank.updateBattleInfo(info);

    // Update the tanks reserved positions
    tanks_reserved_positions_ = info.getTanksReservedPositions();
}