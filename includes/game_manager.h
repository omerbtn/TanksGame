#pragma once

#include <cstddef>
#include <map>
#include <optional>

#include "common/TankAlgorithmFactory.h"
#include "common/PlayerFactory.h"
#include "board.h"

class Tank;
class AlgorithmInterface;
class OutputLogger;
class Player;

class GameManager
{
public:
    GameManager(const PlayerFactory& playerFactory, const TankAlgorithmFactory& algorithmFactory)
        : playerFactory_(playerFactory), algorithmFactory_(algorithmFactory), board_(std::make_unique<Board>()) {
    }

    bool readBoard(const std::string& filename) {
        return board_->load_from_file(filename);
    }

    void run() {
    }

private:
    const PlayerFactory& playerFactory_;
    const TankAlgorithmFactory& algorithmFactory_;
    std::unique_ptr<Board> board_;


    /*void run();
    bool game_over() const;

private:
    void step(OutputLogger& logger);

    Board* board_;
    std::size_t half_steps_count_ = 0;
    std::size_t total_max_steps_;
    std::optional<std::size_t> tie_countdown_;*/
};
