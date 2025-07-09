#pragma once

#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "BattleInfo.h"
#include "TankAlgorithm.h"
#include "cell.h"
#include "smart_battle_info.h"
#include "tank.h"


namespace Algorithm_322573304_322647603
{

// Bring necessary types from UserCommon
using UserCommon_322573304_322647603::Cell;
using UserCommon_322573304_322647603::Direction;
using UserCommon_322573304_322647603::Position;
using UserCommon_322573304_322647603::Tank;

class AlgorithmBase : public TankAlgorithm
{
public:
    virtual ~AlgorithmBase() = default;
    AlgorithmBase(int player_index, int tank_index);

    AlgorithmBase(const AlgorithmBase&) = delete;
    AlgorithmBase& operator=(const AlgorithmBase&) = delete;

    AlgorithmBase(AlgorithmBase&&) = delete;
    AlgorithmBase& operator=(AlgorithmBase&&) = delete;

    virtual ActionRequest getAction() override;
    virtual void updateBattleInfo(BattleInfo& info) override;

protected:
    virtual ActionRequest getActionImpl() = 0;

    virtual void extendBattleInfoProcessing(SmartBattleInfo&) {}
    void handleTankMovement(const ActionRequest action);
    virtual void extendShootActionHandling(const Cell&) {}

    bool hasLineOfSightToOpponent(const Position& start_pos, Direction dir, Position& r_opponent_pos) const;
    bool isShellIncoming(const Position& pos, Position* r_shell_pos = nullptr, Direction* r_shell_possible_dir = nullptr, size_t shell_max_distance = 8) const;
    std::optional<ActionRequest> getEvadeActionIfShellIncoming(size_t shell_max_distance = 8) const; // 8 because our grid may be outdated, and we might need time to evade
    size_t calculateShellPosOffset() const;

    virtual void printTankInfo() const;         // Print tank's known information, for debugging purposes
    virtual void extendPrintTankInfo() const {} // Extend the tank info printing, for derived classes

    int player_index_;
    int tank_index_;
    std::shared_ptr<Tank> tank_;
    std::vector<std::vector<Cell>> grid_;
    std::unordered_map<Position, std::unordered_set<Direction>> shell_possible_directions_;
    std::optional<std::pair<Position, Direction>> last_shell_fired_; // Last shell we fired that wasn't reported yet - (shooting position, direction)
    size_t width_;
    size_t height_;
    size_t turns_till_next_battle_info_ = 0; // Turns until the next GetBattleInfo request
};

} // namespace Algorithm_322573304_322647603