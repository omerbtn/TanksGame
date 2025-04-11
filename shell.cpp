#include "shell.h"

Shell::Shell(int x, int y, Direction direction, Board* board) : MovableObject(x, y, ObjectType::Shell, direction, board) {}

void Shell::move_forward()
{
	move(direction);
}