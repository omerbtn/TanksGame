#include "shell.h"

Shell::Shell(Position position, Direction direction, Board* board) : MovableObject(position, ObjectType::Shell, direction, board) {}

void Shell::move_forward()
{
	move(true);
}