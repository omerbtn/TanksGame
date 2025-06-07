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

    virtual ActionRequest getActionImpl() override;

protected:
    virtual void extendBattleInfoProcessing(SmartBattleInfo &info) override;
    virtual void extendPrintTankInfo() const override;

private:
    bool isShellInPathDangerous(const Position &pos);
    std::optional<ActionRequest> findFirstSafeActionToOpponent();
    void tryForwardMove(std::queue<BFSState> &q,
                        std::unordered_map<BFSState, std::pair<BFSState, ActionRequest>> &parent,
                        std::unordered_set<BFSState> &visited,
                        const BFSState &current);

    std::optional<ActionRequest> handleLineOfSightToOpponent(BFSState &current,
                                                             std::unordered_map<BFSState, std::pair<BFSState, ActionRequest>> &parent,
                                                             const BFSState &start_state);

    std::unordered_set<Position> computeReservedPositions(bool include_shooting_lane = true);

    std::unordered_set<Position> other_tanks_reserved_positions_; // All other tanks reserved positions
    std::queue<ActionRequest> cached_path_;
    Position cached_target_;
};