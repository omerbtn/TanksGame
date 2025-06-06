#include "tank.h"

#include "global_config.h"

Tank::Tank() : MovableObject(Direction::R) {}

Tank::Tank(int player_id, int tank_id, Position position, Direction direction, size_t num_shells)
    : MovableObject(direction), player_id_(player_id), tank_id_(tank_id), position_(position), shells_(num_shells) {}

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

void Tank::destroy()
{
    alive_ = false;
}

void Tank::decreaseCooldown()
{
    if (cooldown_ > 0) cooldown_--;
}

bool Tank::canShoot() const
{
    return cooldown_ == 0 && shells_ > 0 && backwait_ == 0;
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
    backwait_ = 2;
}

void Tank::tickBackwait()
{
    if (backwait_ > 0) --backwait_;
}

void Tank::resetBackwait()
{
    backwait_ = 0;
}

void Tank::continueBacking()
{
    backwait_ = 1;
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