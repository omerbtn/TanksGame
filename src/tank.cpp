#include "tank.h"

Tank::Tank(Position position, Direction direction, Board* board) : MovableObject(position, ObjectType::Tank, direction, board),
															  artillery_shells(16), shoot_cooldown (0), backward_state(BackwardState::None) {}

void Tank::shoot() 
{
	if (is_waiting_for_reverse() || shoot_cooldown != 0 || artillery_shells == 0)
	{
		// Illegal move, do nothing
		do_nothing();
	}
	else
	{
		Shell* shell = new Shell(position, direction, board);  // TODO: Should change, shell should be created in the adjacent cell
		board->addShell(shell);
		shoot_cooldown = 4;
		artillery_shells--;
		cancel_backward();
	}
}

void Tank::move_forward() 
{
	if (!is_waiting_for_reverse()) {
		move(true);
	}	
	// If the tank is in reverse, cancel it
	cancel_backward();
	reduce_shoot_cooldown();
}

void Tank::move_backward()
{
	/*
	if (reverse_counter == 0)
		in_reverse = true;

	if (in_reverse)
	{
		move(false);
	}
	else
		reverse_counter--;

	if (!in_reverse)
		reverse_counter--;
	*/
	update_backward_state();
	if (backward_state == BackwardState::ReadyToMove) {
		move(false);
	}
	reduce_shoot_cooldown();
}

void Tank::update_backward_state() 
{
	if (backward_state == BackwardState::None)
		backward_state = BackwardState::Waiting1;
	else if (backward_state == BackwardState::Waiting1) {
		backward_state = BackwardState::Waiting2;
	}
	else if (backward_state == BackwardState::Waiting2) {
		backward_state = BackwardState::ReadyToMove;
	}
	// If the state is ReadyToMove ,stay in the same state, as the tank can keep moving backward immediately
}

void Tank::rotate(Direction rotate_direction) 
{
	if (is_waiting_for_reverse())
	{
		// Illegal move, do nothing
		do_nothing();
		return;
	}
	
	cancel_backward();
	reduce_shoot_cooldown();
	direction = (Direction)(((int)direction + (int)rotate_direction) % 360);
}

void Tank::do_nothing() 
{
	// If the tank is waiting for reverse, update the state
	// If the tank is not waiting for reverse, cancel the backward state
	if (is_waiting_for_reverse())
		update_backward_state();
	else
		cancel_backward();

	// Anyway, the shoot cooldown should be reduced
	reduce_shoot_cooldown();
}