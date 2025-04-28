#pragma once

#include <cstddef>
#include <map>
#include <optional>

class Board;
class Tank;
class AlgorithmInterface;
class OutputLogger;
class Player;

class GameManager
{
public:
    GameManager(Board* board);

    void run();
    bool game_over() const;

private:
    Board* board_;
    std::size_t half_steps_count_ = 0;
    std::size_t total_max_steps_;
    std::optional<std::size_t> tie_countdown_;

    void step(OutputLogger& logger);
};
