#pragma once

#include <concepts>
#include <type_traits>
#include <unordered_map>

#include "ActionRequest.h"
#include "SatelliteView.h"
#include "cell.h"
#include "types/direction.h"
#include "types/position.h"


// Represents a state in BFS with position and direction
struct BFSState
{
    Position pos;
    Direction dir;
    size_t shells_left;
    size_t cooldown;
    std::unordered_map<Position, size_t> walls_damage; // damage dealt to walls in search

    bool operator==(const BFSState& other) const
    {
        return pos == other.pos && dir == other.dir &&
               shells_left == other.shells_left && cooldown == other.cooldown &&
               walls_damage == other.walls_damage;
    }

    bool operator!=(const BFSState& other) const
    {
        return !(*this == other);
    }
};

// Hash specialization for BFSState to use in unordered containers
namespace std
{
template <>
struct hash<BFSState>
{
    size_t operator()(const BFSState& state) const
    {
        size_t h = 0;
        h ^= hash<Position>()(state.pos);
        h ^= (hash<int>()(static_cast<int>(state.dir)) << 1);
        h ^= (hash<size_t>()(state.shells_left) << 2);
        h ^= (hash<size_t>()(state.cooldown) << 3);

        // Hash the walls_damage map (order-independent)
        size_t wall_hash = 0;
        for (const auto& [pos, dmg] : state.walls_damage)
        {
            wall_hash ^= (hash<Position>()(pos) ^ (hash<size_t>()(dmg) << 1));
        }
        h ^= (wall_hash << 4);

        return h;
    }
};
} // namespace std

// Concept to check if a triple of (Map, Key, Value) is compatible with a map interface
template <typename Map, typename Key, typename Value>
concept CompatibleMap =
    requires(const Map& m, const Key& k) {
        typename Map::key_type;
        typename Map::mapped_type;
        { m.find(k) } -> std::same_as<typename Map::const_iterator>;
        { m.end() } -> std::same_as<typename Map::const_iterator>;
    } &&
    std::same_as<typename Map::key_type, Key> &&
    std::same_as<typename Map::mapped_type, Value>;

// Generic getOrDefault function for maps
template <typename Map, typename Key, typename Value>
    requires CompatibleMap<Map, Key, Value>
Value getOrDefault(const Map& map, const Key& key, const Value& default_value)
{
    auto it = map.find(key);
    if (it != map.end())
    {
        return it->second;
    }
    return default_value;
}

void printGrid(const std::vector<std::vector<Cell>>& grid);

const std::vector<Direction>& getAllDirections();
Direction getOppositeDirection(Direction dir);
Direction getDirectionAfterRotation(Direction dir, ActionRequest action);
std::string directionToString(Direction dir);
std::string directionToArrow(Direction dir);
std::string tankActionToString(ActionRequest action);
Direction getSeedDirection(int player_index);

Position forwardPosition(const Position& pos, Direction dir, size_t width, size_t height, size_t steps = 1);
Position backwardPosition(const Position& pos, Direction dir, size_t width, size_t height, size_t steps = 1);
size_t getDistance(const Position& from, const Position& to, Direction dir, size_t width, size_t height);

size_t getNumberOfShellsInGrid(const std::vector<std::vector<Cell>>& grid);
bool isBlockedByWall(const std::vector<std::vector<Cell>>& grid, const Position& from, Direction dir, size_t steps);

std::vector<std::vector<Cell>> reconstructGridFromSatelliteView(const SatelliteView& satellite_view, size_t height, size_t width,
                                                                int player_index, size_t num_shells, Position& r_tank_pos);