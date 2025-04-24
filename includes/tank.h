#pragma once

#include "game_object_interface.h"
#include "types/direction.h"
#include "types/position.h"

class Tank : public GameObjectInterface
{

public:
    Tank(size_t id, Position position, Direction direction);

    Position& position();
    const Position& position() const;
    Direction& direction();
    const Direction& direction() const;

    size_t id() const;
    bool is_alive() const;
    size_t ammo() const;
    void destroy();
    void decrease_cooldown();
    bool can_shoot() const;
    void shoot();
    bool is_backing() const;
    void start_backwait();
    void tick_backwait();
    void reset_backwait();
    void continue_backing();
    bool ready_to_move_back() const;

private:
    virtual ObjectType type() const override;

    Position position_;
    Direction direction_;
    size_t id_;
    size_t shells_ = 16;
    size_t cooldown_ = 0;
    size_t backwait_ = 0;
    bool alive_ = true;
};
