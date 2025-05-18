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

int Tank::id() const
{
    return player_id_;
}

bool Tank::is_alive() const
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

void Tank::decrease_cooldown()
{
    if (cooldown_ > 0) cooldown_--;
}

bool Tank::can_shoot() const
{
    return cooldown_ == 0 && shells_ > 0 && backwait_ == 0;
}

void Tank::shoot()
{
    cooldown_ = 4;
    shells_--;
}

bool Tank::is_backing() const
{
    return backwait_ > 0;
}

void Tank::start_backwait()
{
    backwait_ = 2;
}

void Tank::tick_backwait()
{
    if (backwait_ > 0) --backwait_;
}

void Tank::reset_backwait()
{
    backwait_ = 0;
}

void Tank::continue_backing()
{
    backwait_ = 1;
}

bool Tank::ready_to_move_back() const
{
    return backwait_ == 0;
}
