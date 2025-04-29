#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "game_object_interface.h"
#include "tank.h"
#include "shell.h"
#include "wall.h"
#include "mine.h"
#include "types/position.h"

class Cell
{
public:
    Cell() = default;
    Cell(Position position, std::shared_ptr<GameObjectInterface> object = nullptr);

    Position& position();
    const Position& position() const;

    void add_object(std::shared_ptr<GameObjectInterface> obj);
    void remove_object(std::shared_ptr<GameObjectInterface> obj);
    void remove_objects_by_type(ObjectType type);

    std::shared_ptr<GameObjectInterface> get_object_by_type(ObjectType type) const;
    const std::vector<std::shared_ptr<GameObjectInterface>>& get_objects_by_type(ObjectType type) const;
    size_t get_objects_count() const;
    
    bool has(ObjectType type) const;
    bool empty() const;

private:
    Position position_;
    std::unordered_map<ObjectType, std::vector<std::shared_ptr<GameObjectInterface>>> objects_;
};
