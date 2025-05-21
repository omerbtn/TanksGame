#pragma once

#include <utility>
#include <memory>

using Position = std::pair<size_t, size_t>;

// Print Position as (x, y)
inline std::ostream& operator<<(std::ostream& os, const Position& pos) 
{
    return os << '(' << pos.first << ", " << pos.second << ')';
}

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
}
