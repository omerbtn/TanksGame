#include "tank.h"

#include "global_config.h"


Tank::Tank() : MovableObject(Direction::R) {}

Tank::Tank(int player_id, int tank_id, Position position, Direction direction, size_t num_shells)
    : MovableObject(direction), tank_id_(tank_id), player_id_(player_id), position_(position), shells_(num_shells) {}

ObjectType Tank::type() const
{
    return ObjectType::Tank;
}

Position& Tank::position()
{
    return position_;
}

const Position& Tank::position() const
{
    return position_;
}

bool Tank::isAlive() const
{
    return alive_;
}

size_t Tank::ammo() const
{
    return shells_;
}

size_t Tank::cooldown() const
{
    return cooldown_;
}

void Tank::destroy()
{
    alive_ = false;
}

void Tank::decreaseCooldown()
{
    if (cooldown_ > 0)
        cooldown_--;
}

bool Tank::canShoot() const
{
    return cooldown_ == 0 && shells_ > 0 && backwait_ == 0 && !waiting_back_move_;
}

void Tank::shoot()
{
    cooldown_ = 4;
    shells_--;
}

bool Tank::isBacking() const
{
    return backwait_ > 0;
}

void Tank::startBackwait()
{
    backwait_ = 1;
    waiting_back_move_ = true;
}

void Tank::tickBackwait()
{
    if (backwait_ > 0)
        --backwait_;
}

void Tank::resetBackwait()
{
    backwait_ = 0;
    waiting_back_move_ = false;
}

bool Tank::readyToMoveBack() const
{
    return backwait_ == 0;
}

void Tank::copyRuntimeStateFrom(const Tank& other)
{
    position_ = other.position_;
    direction_ = other.direction_;
    shells_ = other.shells_;
    cooldown_ = other.cooldown_;
    backwait_ = other.backwait_;
    alive_ = other.alive_;
}

bool Tank::waitingBackMove() const
{
    return waiting_back_move_;
}

void Tank::setWaitingBackMove(bool waiting_back_move)
{
    waiting_back_move_ = waiting_back_move;
}

ActionRequest Tank::lastAction() const
{
    return last_action_;
}

void Tank::setLastAction(ActionRequest action)
{
    last_action_ = action;
}