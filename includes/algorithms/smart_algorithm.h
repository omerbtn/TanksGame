#pragma once

#include <queue>
#include <unordered_map>

#include "algorithm_base.h"
#include "smart_battle_info.h"

struct BFSState;

class SmartAlgorithm : public AlgorithmBase
{
public:
    virtual ~SmartAlgorithm() = default;
    SmartAlgorithm(int player_index, int tank_index);

    SmartAlgorithm(const SmartAlgorithm&) = delete;
    SmartAlgorithm& operator=(const SmartAlgorithm&) = delete;

    SmartAlgorithm(SmartAlgorithm&&) = delete;
    SmartAlgorithm& operator=(SmartAlgorithm&&) = delete;

    virtual ActionRequest getActionImpl() override;

protected:
    virtual void extendBattleInfoProcessing(SmartBattleInfo& info) override;
    virtual void extendPrintTankInfo() const override;

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

    ActionRequest handleLineOfSightToOpponent(BFSState& current,
                                              std::unordered_map<BFSState, std::pair<BFSState, ActionRequest>>& parent,
                                              const BFSState& start_state,
                                              const Position& opponent_pos);

    std::unordered_set<Position> computeReservedPositions(bool include_shooting_lane = true);

    std::unordered_set<Position> other_tanks_reserved_positions_; // All other tanks reserved positions
    std::queue<ActionRequest> cached_path_;
    Position cached_target_;
};