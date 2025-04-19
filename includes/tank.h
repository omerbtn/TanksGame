#pragma once

#include "movableObject.h"
#include "direction.h"

// forward declaration
class Board;

enum class BackwardState {
    None,         // No backward movement
    Waiting1,     // 1st step waiting
    Waiting2,     // 2nd step waiting
    ReadyToMove,  // Ready to move backward
};

class Tank : public MovableObject {
private:
    int artillery_shells;
    int shoot_cooldown;
	//bool in_reverse;      // Should be true if the tank is in reverse, i.e. he can move backward freely  
    //int reverse_counter;  // Should be decreased every turn
    BackwardState backward_state; // State of the tank when moving backward, should be updated every turn

public:
    Tank(Position position, Direction direction, Board* board);

    void shoot();
    void move_forward();
    void move_backward();
    void rotate(Direction rotate_direction);
    void do_nothing();

private:
    void cancel_backward() { backward_state = BackwardState::None; };
    void update_backward_state();
    void reduce_shoot_cooldown() { if (shoot_cooldown > 0) shoot_cooldown--; };
    bool is_waiting_for_reverse() const { return backward_state == BackwardState::Waiting1 || backward_state == BackwardState::Waiting2; };
};


