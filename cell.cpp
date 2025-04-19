#include "cell.h"

Cell::Cell(Position position, GameObject* object) : position{position} {}
Position Cell::getPosition() const { 
	return position; 
}
int Cell::getX() const { 
	return position.x;
}
int Cell::getY() const { 
	return position.y; 
}
bool Cell::add_object(GameObject* obj) {
	return false; // TODO: return something that makes sense
}
bool Cell::is_tank() const { 
	return tank == nullptr; 
}
bool Cell::is_wall() const {
	return wall == nullptr; 
}
bool Cell::is_shell() const {
	return shell == nullptr; 
}
bool Cell::is_mine() const {
	return mine == nullptr; 
}