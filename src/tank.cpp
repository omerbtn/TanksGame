#include "tank.h"

#include "global_config.h"

Tank::Tank(size_t id, Position position, Direction direction)
    : MovableObject{direction}, id_{id}, position_{position}, shells_{config::get<size_t>("shell_per_tank")} {
}

ObjectType Tank::type() const {
    return ObjectType::Tank;
}

Position& Tank::position() {
    return position_;
}

const Position& Tank::position() const {
    return position_;
}

size_t Tank::id() const {
    return id_;
}

bool Tank::is_alive() const {
    return alive_;
}

size_t Tank::ammo() const {
    return shells_;
}

void Tank::destroy() {
    alive_ = false;
}

void Tank::decrease_cooldown() {
    if (cooldown_ > 0) cooldown_--;
}

bool Tank::can_shoot() const {
    return cooldown_ == 0 && shells_ > 0 && backwait_ == 0;
}

void Tank::shoot() {
    cooldown_ = 4;
    shells_--;
}

bool Tank::is_backing() const {
    return backwait_ > 0;
}

void Tank::start_backwait() {
    backwait_ = 2;
}

void Tank::tick_backwait() {
    if (backwait_ > 0) --backwait_;
}

void Tank::reset_backwait() {
    backwait_ = 0;
}

void Tank::continue_backing() {
    backwait_ = 1;
}

bool Tank::ready_to_move_back() const {
    return backwait_ == 0;
}
