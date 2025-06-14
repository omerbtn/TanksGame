#pragma once

#include "movable_object.h"
#include "types/direction.h"
#include "types/position.h"
#include "ActionRequest.h"

class Tank : public MovableObject
{
public:
    Tank();
    Tank(int player_id, int tank_id, Position position, Direction direction, size_t num_shells);

    Tank(const Tank&) = delete;
    Tank& operator=(const Tank&) = delete;
    Tank(Tank&&) = delete;
    Tank& operator=(Tank&&) = delete;

    Position& position();
    const Position& position() const;
    int playerId() const { return player_id_; }
    int tankId() const { return tank_id_; }
    bool isAlive() const;
    size_t ammo() const;
    void destroy();
    void decreaseCooldown();
    bool canShoot() const;
    void shoot();
    bool isBacking() const;
    void startBackwait();
    void tickBackwait();
    void resetBackwait();
    bool readyToMoveBack() const;
    bool waitingBackMove() const;
    void setWaitingBackMove(bool waiting_back_move);
    void copyRuntimeStateFrom(const Tank& other);
    ActionRequest lastAction() const;
    void setLastAction(ActionRequest action);

private:
    virtual ObjectType type() const override;

    int tank_id_;
    int player_id_;
    Position position_;
    size_t shells_;
    size_t cooldown_ = 0;
    size_t backwait_ = 0;
    bool alive_ = true;
    bool waiting_back_move_ = false;
    ActionRequest last_action_ = ActionRequest::DoNothing;
};
