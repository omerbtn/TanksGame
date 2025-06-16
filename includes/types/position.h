#pragma once

#include <iostream>
#include <memory>
#include <utility>

using Position = std::pair<size_t, size_t>;

// Print Position as (x, y)
inline std::ostream& operator<<(std::ostream& os, const Position& pos)
{
    return os << '(' << pos.first << ", " << pos.second << ')';
}

// Hash specialization for Position and std::pair<Position, Position>,
// to be used in unordered containers.
namespace std
{
template <>
struct hash<Position>
{
    size_t operator()(const Position& pos) const
    {
        return hash<size_t>()(pos.first) ^ (hash<size_t>()(pos.second) << 1);
    }
};

template <>
struct hash<std::pair<Position, Position>>
{
    size_t operator()(const std::pair<Position, Position>& p) const
    {
        size_t h1 = hash<Position>()(p.first);
        size_t h2 = hash<Position>()(p.second);

        return h1 ^ (h2 << 1);
    }
};
} // namespace std
