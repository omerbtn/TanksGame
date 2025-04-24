#include "cell.h"

Cell::Cell(Position position, std::shared_ptr<GameObjectInterface> object) : position_(position) {
    if (object) {
        objects_[object->type()] = object;
    }
}

Position& Cell::position() {
    return position_;
}

const Position& Cell::position() const {
    return position_;
}

std::unordered_map<ObjectType, std::shared_ptr<GameObjectInterface>>& Cell::objects() {
    return objects_;
}

const std::unordered_map<ObjectType, std::shared_ptr<GameObjectInterface>>& Cell::objects() const {
    return objects_;
}

void Cell::add_object(std::shared_ptr<GameObjectInterface> object) {
    if (object) {
        objects_[object->type()] = object;
    }
}

void Cell::remove_object(ObjectType type) {
    objects_.erase(type);
}

std::shared_ptr<GameObjectInterface> Cell::get_object(ObjectType type) const {
    auto it = objects_.find(type);
    return it != objects_.end() ? it->second : nullptr;
}

bool Cell::has(ObjectType type) const {
    return objects_.find(type) != objects_.end();
}

bool Cell::empty() const {
    return objects_.empty();
}
