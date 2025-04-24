#pragma once

#include <memory>
#include <unordered_map>

#include "game_object_interface.h"
#include "types/position.h"
class Cell
{
public:
    Cell() = default;
    Cell(Position position, std::shared_ptr<GameObjectInterface> object = nullptr);

    Position& position();
    const Position& position() const;
    std::unordered_map<ObjectType, std::shared_ptr<GameObjectInterface>>& objects();
    const std::unordered_map<ObjectType, std::shared_ptr<GameObjectInterface>>& objects() const;

    void add_object(std::shared_ptr<GameObjectInterface> obj);
    void remove_object(ObjectType type);
    std::shared_ptr<GameObjectInterface> get_object(ObjectType type) const;
    bool has(ObjectType type) const;
    bool empty() const;

private:
    Position position_;
    std::unordered_map<ObjectType, std::shared_ptr<GameObjectInterface>> objects_;
};
