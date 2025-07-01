#include "players/simple_player.h"

#include "PlayerRegistration.h"
#include "smart_battle_info.h"
#include "utils.h"


SimplePlayer::SimplePlayer(int player_index, size_t x, size_t y, size_t max_steps, size_t num_shells)
    : PlayerBase(player_index, x, y, max_steps, num_shells) {}

void SimplePlayer::updateTankWithBattleInfo(TankAlgorithm& tank, SatelliteView& satellite_view)
{
    SmartBattleInfo info = createBattleInfo(satellite_view);
    tank.updateBattleInfo(info);
}