#pragma once

#include <memory>
#include <vector>

#include "TankAlgorithm.h"
#include "cell.h"
#include "tank.h"

class BattleInfo;

class AlgorithmBase : public TankAlgorithm 
{
public:
    virtual ~AlgorithmBase() = default;
    AlgorithmBase(int player_index, int tank_index);

    virtual ActionRequest getAction() override;
    virtual void updateBattleInfo(BattleInfo& info) override;

protected:
    virtual ActionRequest getActionImpl() = 0;

    void handle_tank_movement(const ActionRequest action);
    std::shared_ptr<Tank> getClosestOpponent() const;
    bool hasLineOfSightToOpponent(const Position& start_pos, Direction dir, Position& r_opponent_pos) const;

    int player_index_;
    int tank_index_;
    std::shared_ptr<Tank> tank_;
    std::vector<std::vector<Cell>> grid_;
    size_t width_;
    size_t height_;

    int should_request_info_ = 0;
    bool tank_created_ = false;
};
