#include "tank.h"

Tank::Tank(int x, int y, Direction direction, Board* board) : MovableObject(x, y, ObjectType::Tank, direction, board), artilleryShells(16), shoot_counter (0){}

void Tank::shoot() {
	if (shoot_counter != 0) return;
	Shell* shell = new Shell(x, y, direction, board);
	board->addShell(shell);
	shoot_counter = 4;

}

void Tank::move_backward()
{
	if (reverse_counter == 0)
		in_reverse = true;

	if (in_reverse)
	{
		move()
	}
	else
		reverse_counter = 3;

	if (!in_reverse)
		reverse_counter--;
}

void Tank::rotate(RotateDirection rotateDirection) {
	direction = (Direction)(((int)direction + (int)rotateDirection + 360) % 360);
}