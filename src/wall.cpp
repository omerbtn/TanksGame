#include "wall.h"

Wall::Wall(Position position) : GameObject(position, ObjectType::Wall) ,durability (2){}

bool Wall::hit() {
	return --durability == 0;
}