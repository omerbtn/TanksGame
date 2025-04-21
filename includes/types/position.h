#pragma once

#include <utility>
#include <memory>

using Position = std::pair<int, int>;

namespace std
{
    template <>
    struct hash<Position>
    {
        size_t operator()(const Position &pos) const
        {
            return hash<int>()(pos.first) ^ (hash<int>()(pos.second) << 1);
        }
    };
}