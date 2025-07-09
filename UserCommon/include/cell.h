#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "game_object_interface.h"
#include "types/position.h"


namespace UserCommon_322573304_322647603
{

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

} // namespace UserCommon_322573304_322647603