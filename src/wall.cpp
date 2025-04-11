#include "wall.h"

Wall::Wall(int x, int y) : GameObject(x, y, ObjectType::Wall) ,durability (2){}

bool Wall::hit() {
	return --durability == 0;
}