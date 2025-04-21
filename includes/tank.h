#pragma once

#include "game_object_interface.h"
#include "types/direction.h"
#include "types/position.h"

class Tank : public GameObjectInterface
{
    int id_;
    int shells_ = 16;
    int cooldown_ = 0;
    int backwait_ = 0;
    bool alive_ = true;

public:
    Position pos;
    Direction dir;

    Tank(int id, Position p, Direction d) : id_(id), pos(p), dir(d) {}
    ObjectType type() const override { return ObjectType::Tank; }

    int id() const {
        return id_;
    }
    bool isAlive() const {
        return alive_;
    }
    int ammo() const {
        return shells_;
    }
    void destroy() {
        alive_ = false;
    }
    void decreaseCooldown() {
        if (cooldown_ > 0) cooldown_--;
    }
    bool canShoot() const {
        return cooldown_ == 0 && shells_ > 0;
    }
    void shoot() {
        cooldown_ = 4;
        shells_--;
    }
    bool isBacking() const {
        return backwait_ > 0;
    }
    void startBackwait() {
        backwait_ = 2;
    }
    void tickBackwait() {
        if (backwait_ > 0) --backwait_;
    }
    bool readyToMoveBack() const {
        return backwait_ == 0;
    }
};
