#include "movableObject.h"

MovableObject::MovableObject(int x, int y, ObjectType type, Direction direction, Board* board) : GameObject(x, y, type), direction(direction), board(board) {}

void MovableObject::move(bool is_forward = true) {
	Cell* next_cell = nullptr;
	Direction move_dircetion = direction;
	
	if (!is_forward) {
		move_dircetion = (Direction)(((int)direction + 180 + 360) % 360);
	}

	switch (move_dircetion)
	{
	case Direction::U:
		next_cell = board->getCell(x, y - 1);
		break;
	case Direction::UR:
		next_cell = board->getCell(x+1, y - 1);
		break;
	case Direction::R:
		next_cell = board->getCell(x + 1, y);
		break;
	case Direction::DR:
		next_cell = board->getCell(x + 1, y + 1);
		break;
	case Direction::D:
		next_cell = board->getCell(x, y + 1);
		break;
	case Direction::DL:
		next_cell = board->getCell(x - 1, y + 1);
		break;
	case Direction::L:
		next_cell = board->getCell(x - 1, y);
		break;
	case Direction::UL:
		next_cell = board->getCell(x - 1, y - 1);
		break;
	default:
		break;
	}
	next_cell->add_obj(this);
	x = next_cell->getX();
	y = next_cell->getY();
}