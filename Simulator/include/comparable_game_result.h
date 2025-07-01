#pragma once

#include <memory.h>
#include <string>

#include "GameResult.h"
#include "utils.h"


// Comparable GameResult, used for comparing GameManagers runs results
struct ComparableGameResult
{
    std::shared_ptr<GameResult> result;
    std::string final_state_str; // String representation of the final game state

    ComparableGameResult(GameResult&& res, size_t width, size_t height)
        : result(std::make_shared<GameResult>(std::move(res))),
          final_state_str(satelliteViewToString(*result->game_state, width, height)) {}

    bool operator==(const ComparableGameResult& other) const
    {
        return result->winner == other.result->winner &&
               result->reason == other.result->reason &&
               result->rounds == other.result->rounds &&
               final_state_str == other.final_state_str;
    }
};

// Required for unordered_map
namespace std
{
template <>
struct hash<ComparableGameResult>
{
    size_t operator()(const ComparableGameResult& result) const
    {
        size_t h = 0;
        h ^= hash<int>()(result.result->winner);
        h ^= (hash<int>()(static_cast<int>(result.result->reason)) << 1);
        h ^= (hash<size_t>()(result.result->rounds) << 2);
        h ^= hash<std::string>()(result.final_state_str);
        return h;
    }
};
} // namespace std