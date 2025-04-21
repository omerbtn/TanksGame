#pragma once

#include <memory>

#include "game_object_interface.h"
#include "types/position.h"
class Cell
{
public:
    Cell() = default;
    Cell(Position pos, std::shared_ptr<GameObjectInterface> obj = nullptr) : pos(pos), object(std::move(obj)) {}

    Position pos;
    std::shared_ptr<GameObjectInterface> object = nullptr;
};