#pragma once

#include <queue>
#include <unordered_map>

#include "algorithm_base.h"
#include "smart_battle_info.h"
#include "utils.h"


namespace Algorithm_322573304_322647603
{

// Bring necessary types from UserCommon
using UserCommon_322573304_322647603::BFSState;

// Used to be SmartAlgorithm
class TankAlgorithm_322573304_322647603 : public AlgorithmBase
{
public:
    virtual ~TankAlgorithm_322573304_322647603() = default;
    TankAlgorithm_322573304_322647603(int player_index, int tank_index);

    TankAlgorithm_322573304_322647603(const TankAlgorithm_322573304_322647603&) = delete;
    TankAlgorithm_322573304_322647603& operator=(const TankAlgorithm_322573304_322647603&) = delete;

    TankAlgorithm_322573304_322647603(TankAlgorithm_322573304_322647603&&) = delete;
    TankAlgorithm_322573304_322647603& operator=(TankAlgorithm_322573304_322647603&&) = delete;

    virtual ActionRequest getActionImpl() override;

protected:
    virtual void extendBattleInfoProcessing(SmartBattleInfo& info) override;
    virtual void extendPrintTankInfo() const override;
    virtual void extendShootActionHandling(const Cell& next_cell) override;

private:
    std::optional<ActionRequest> findFirstSafeActionToOpponent();

    void tryForwardMove(std::queue<BFSState>& q,
                        std::unordered_map<BFSState, std::pair<BFSState, ActionRequest>>& parent,
                        std::unordered_set<BFSState>& visited,
                        const BFSState& current);

    void tryRotations(std::queue<BFSState>& q,
                      std::unordered_map<BFSState, std::pair<BFSState, ActionRequest>>& parent,
                      std::unordered_set<BFSState>& visited,
                      const BFSState& current);

    void tryGetBattleInfo(std::queue<BFSState>& q,
                          std::unordered_map<BFSState, std::pair<BFSState, ActionRequest>>& parent,
                          std::unordered_set<BFSState>& visited,
                          const BFSState& current);

    void tryShootingAWall(std::queue<BFSState>& q,
                          std::unordered_map<BFSState, std::pair<BFSState, ActionRequest>>& parent,
                          std::unordered_set<BFSState>& visited,
                          const BFSState& current);

    bool isCellEmptyInState(const BFSState& state, const Position& pos) const;

    ActionRequest handleLineOfSightToOpponent(BFSState& current,
                                              std::unordered_map<BFSState, std::pair<BFSState, ActionRequest>>& parent,
                                              const BFSState& start_state,
                                              const Position& opponent_pos);

    std::unordered_set<Position> computeReservedPositions(bool include_shooting_lane = true);

    std::unordered_set<Position> other_tanks_reserved_positions_; // All other tanks reserved positions
    std::queue<ActionRequest> cached_path_;
    Position cached_target_;
    std::unordered_map<Position, size_t> total_walls_damage_; // Wall's position -> number of hits it has taken
    std::unordered_map<Position, size_t> local_walls_damage_; // Wall's position -> number of hits we made to it since last GetBattleInfo
};

} // namespace Algorithm_322573304_322647603