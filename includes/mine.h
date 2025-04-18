#pragma once
#include "gameObject.h"

class Mine : public GameObject 
{
public:
	Mine(Position position) : GameObject(position, ObjectType::Mine) {}
};
