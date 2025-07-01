#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "game_object_interface.h"
#include "mine.h"
#include "shell.h"
#include "tank.h"
#include "types/position.h"
#include "wall.h"

class Cell
{
public:
    Cell() = default;
    Cell(Position position, std::shared_ptr<GameObjectInterface> object = nullptr);

    Position& position();
    const Position& position() const;

    void addObject(std::shared_ptr<GameObjectInterface> obj);
    void removeObject(std::shared_ptr<GameObjectInterface> obj);
    void removeObjectsByType(ObjectType type);

    std::shared_ptr<GameObjectInterface> getObjectByType(ObjectType type) const;
    const std::vector<std::shared_ptr<GameObjectInterface>>& getObjectsByType(ObjectType type) const;
    size_t getObjectsCount() const;

    bool has(ObjectType type) const;
    bool empty() const;

    char toChar() const;

private:
    Position position_;
    std::unordered_map<ObjectType, std::vector<std::shared_ptr<GameObjectInterface>>> objects_;
};
