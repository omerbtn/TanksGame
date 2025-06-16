#pragma once

#include <cstddef>
#include <map>
#include <optional>

#include "ActionRequest.h"
#include "Player.h"
#include "TankAlgorithm.h"
#include "board.h"
#include "game_info.h"
#include "output_logger.h"
#include "tank.h"


class GameManager
{
public:
    GameManager(const PlayerFactory& playerFactory, const TankAlgorithmFactory& algorithmFactory);

    GameManager(const GameManager&) = delete;
    GameManager& operator=(const GameManager&) = delete;
    GameManager(GameManager&&) = delete;
    GameManager& operator=(GameManager&&) = delete;

    bool readBoard(const std::string& filename);
    void run();

private:
    static std::pair<std::string, std::string> splitFilename(const std::string& filename);
    bool isGameOver() const;
    void doTanksStep();
    void getTanksActions();
    void checkActionsValidity();
    void handleTie();
    std::string generateResultMessage() const;
    void logTankActions();

    std::unique_ptr<Board> board_;
    std::vector<std::shared_ptr<Tank>> ordered_tanks_;
    size_t total_max_steps_;
    OutputLogger logger_;
    std::optional<std::size_t> tie_countdown_;
    size_t half_steps_count_ = 0;
    std::vector<bool> was_alive_at_round_start_;
    std::vector<std::optional<ActionRequest>> actions_to_execute_;
    std::vector<bool> actions_validity_;
};
