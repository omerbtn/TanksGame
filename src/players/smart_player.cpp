#include "smart_player.h"

#include "algorithm_utils.h"
#include "global_config.h"
#include "smart_battle_info.h"


SmartPlayer::SmartPlayer(int player_index, size_t x, size_t y, size_t max_steps, size_t num_shells)
    : PlayerBase(player_index, x, y, max_steps, num_shells) {}

void SmartPlayer::updateTankWithBattleInfo(TankAlgorithm& tank, SatelliteView& satellite_view)
{
    SmartBattleInfo info = createBattleInfo(satellite_view);

    // Update our walls damage
    updateWallsDamage();

    // Extend the battle info with reserved positions and walls damage
    info.setTanksReservedPositions(tanks_reserved_positions_);
    info.setWallsDamage(walls_damage_, true);

    tank.updateBattleInfo(info);

    // Update the tanks reserved positions
    tanks_reserved_positions_ = info.getTanksReservedPositions();

    // Update the walls damage
    walls_damage_ = info.getWallsDamage();
}

// Derives damage made to walls by shells that are close to them, we are certain about their direction,
// and we can tell with high confidence that they will hit the wall.
// Also makes sure we don't report the same shell hitting the wall multiple times, even after shell moves.
void SmartPlayer::updateWallsDamage()
{
    for (const auto& [shell_pos, directions] : shell_possible_directions_)
    {
        if (directions.size() > 1)
        {
            // If there are multiple possible directions, we can't be sure which wall is going to be hitten
            continue;
        }

        Direction shell_dir = *directions.begin();
        Position wall_pos;
        if (isShellCloseToWall(shell_pos, shell_dir, wall_pos))
        {
            // We need to make sure we haven't already reported this shell hitting the wall
            for (size_t turns_passed : possible_turns_passed_)
            {
                Position prev_shell_pos = backwardPosition(shell_pos, shell_dir, width_, height_, turns_passed * 2);
                if (reported_shell_wall_hits_.count({prev_shell_pos, wall_pos}) == 0)
                {
                    // If we haven't reported this shell hitting the wall, we can update the damage
                    walls_damage_[wall_pos]++;                               // Increment the damage for the wall
                    reported_shell_wall_hits_.insert({shell_pos, wall_pos}); // Mark this shell-wall hit as reported
                }
                else
                {
                    // We already reported this shell hitting the wall, no need to update the walls damage,
                    // but need to update the reported shell-wall hits according to the current shell position
                    reported_shell_wall_hits_.erase({prev_shell_pos, wall_pos});
                    reported_shell_wall_hits_.insert({shell_pos, wall_pos});
                }
            }
        }
    }

    // Cleanup: Remove shell-wall hits for shells that no longer exist
    for (auto it = reported_shell_wall_hits_.begin(); it != reported_shell_wall_hits_.end();)
    {
        const Position& shell_pos = it->first;
        if (grid_[shell_pos.first][shell_pos.second].has(ObjectType::Shell))
        {
            ++it; // Shell still exists, keep the hit
        }
        else
        {
            it = reported_shell_wall_hits_.erase(it); // Shell no longer exists, remove the hit
        }
    }
}

bool SmartPlayer::isShellCloseToWall(const Position& shell_pos, Direction shell_dir, Position& r_wall_pos) const
{
    if (grid_.empty() || grid_[0].empty())
    {
        return false; // If grid is empty, no walls can be close
    }

    size_t steps = config::get<size_t>("shells_close_to_wall_distance");
    Position pos = shell_pos;

    for (size_t i = 0; i < steps; ++i)
    {
        pos = forwardPosition(pos, shell_dir, width_, height_);
        const Cell& cell = grid_[pos.first][pos.second];

        if (cell.has(ObjectType::Wall))
        {
            r_wall_pos = pos; // Output the position of the wall
            return true;      // Found a wall in the direction of the shell
        }
        else if (cell.has(ObjectType::Tank))
        {
            return false; // A tank is in the way, we can't be sure it'll move
        }
        else if (cell.has(ObjectType::Shell))
        {
            if (getOrDefault(shell_possible_directions_, pos, std::unordered_set<Direction>()).count(getOppositeDirection(shell_dir)))
            {
                // If there's a shell moving in the opposite direction, it'll hit us and we won't make it to the wall
                return false;
            }
            // Else, the shell is going away and won't be there when we pass
        }
    }

    return false; // No wall found
}
