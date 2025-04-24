#pragma once

#include <cstddef>
#include <map>

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

private:
    Board* board_;
    std::size_t step_count_;
    std::size_t tie_countdown_;

    void step(OutputLogger& logger);

    bool game_over() const;
};
