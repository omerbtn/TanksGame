#include "cell.h"

#include <algorithm>


Cell::Cell(Position position, std::shared_ptr<GameObjectInterface> object) : position_(position) 
{
    if (object) 
    {
        objects_[object->type()].push_back(object);
    }
}

Position& Cell::position() 
{
    return position_;
}

const Position& Cell::position() const 
{
    return position_;
}

void Cell::add_object(std::shared_ptr<GameObjectInterface> object) 
{
    if (object) 
    {
        objects_[object->type()].push_back(object);
    }
}

void Cell::remove_object(std::shared_ptr<GameObjectInterface> object) 
{
    if (object) 
    {
        auto it = objects_.find(object->type());
        if (it != objects_.end()) 
        {
            auto& vec = it->second;
            auto obj_it = std::find(vec.begin(), vec.end(), object);
            if (obj_it != vec.end()) 
            {
                vec.erase(obj_it);
                if (vec.empty()) 
                {
                    objects_.erase(it);
                }
            }
        }
    }
}

// Remove all objects of the specified type, can be usable sometimes
void Cell::remove_objects_by_type(ObjectType type) 
{
    objects_.erase(type);
}

// Return the first object of the specified type, don't use unless you know what you're doing
std::shared_ptr<GameObjectInterface> Cell::get_object_by_type(ObjectType type) const 
{
    auto it = objects_.find(type);
    if (it != objects_.end() && !it->second.empty()) 
    {
        return it->second[0];
    }
    return nullptr;
}

bool Cell::has(ObjectType type) const 
{
    return objects_.find(type) != objects_.end();
}

bool Cell::empty() const 
{
    return objects_.empty();
}

const std::vector<std::shared_ptr<GameObjectInterface>>& Cell::get_objects_by_type(ObjectType type) const
{
    static const std::vector<std::shared_ptr<GameObjectInterface>> empty_vector;
    auto it = objects_.find(type);
    if (it != objects_.end()) 
    {
        return it->second;
    }
    return empty_vector;  // Return an empty vector if the type is not found
}

size_t Cell::get_objects_count() const
{
    size_t count = 0;
    for (const auto& [type, objects] : objects_) 
    {
        count += objects.size();
    }
    return count;
}