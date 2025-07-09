#pragma once

#include <iostream>
#include <memory>
#include <utility>


namespace UserCommon_322573304_322647603
{

using Position = std::pair<size_t, size_t>;

// Print Position as (x, y)
inline std::ostream& operator<<(std::ostream& os, const Position& pos)
{
    return os << '(' << pos.first << ", " << pos.second << ')';
}

} // namespace UserCommon_322573304_322647603


// Hash specialization for Position and std::pair<Position, Position>,
// to be used in unordered containers.
namespace std
{

template <>
struct hash<UserCommon_322573304_322647603::Position>
{
    size_t operator()(const UserCommon_322573304_322647603::Position& pos) const
    {
        return hash<size_t>()(pos.first) ^ (hash<size_t>()(pos.second) << 1);
    }
};

template <>
struct hash<std::pair<UserCommon_322573304_322647603::Position, UserCommon_322573304_322647603::Position>>
{
    size_t operator()(const std::pair<UserCommon_322573304_322647603::Position, UserCommon_322573304_322647603::Position>& p) const
    {
        size_t h1 = hash<UserCommon_322573304_322647603::Position>()(p.first);
        size_t h2 = hash<UserCommon_322573304_322647603::Position>()(p.second);

        return h1 ^ (h2 << 1);
    }
};

} // namespace std
