#pragma once

#include "movable_object.h"
#include "types/direction.h"
#include "types/position.h"


class Tank : public MovableObject
{
public:
    Tank(int player_id, int tank_id, Position position, Direction direction, size_t num_shells);

    Position& position();
    const Position& position() const;

    int id() const;  // Keeping just for compatibility with the old code, should replace with player_id() as this is just confusing
    int player_id() const { return player_id_; }
    int tank_id() const { return tank_id_; }
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

    int tank_id_;
    int player_id_;
    Position position_;
    size_t shells_;
    size_t cooldown_ = 0;
    size_t backwait_ = 0;
    bool alive_ = true;
};
