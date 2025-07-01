#pragma once

#include <cstddef>
#include <map>
#include <optional>

#include "AbstractGameManager.h"
#include "ActionRequest.h"
#include "Player.h"
#include "TankAlgorithm.h"
#include "board.h"
#include "game_info.h"
#include "output_logger.h"
#include "tank.h"


class GameManager : public AbstractGameManager
{
public:
    GameManager(bool verbose = false);

    GameManager(const GameManager&) = delete;
    GameManager& operator=(const GameManager&) = delete;
    GameManager(GameManager&&) = delete;
    GameManager& operator=(GameManager&&) = delete;


    GameResult run(size_t map_width, size_t map_height,
                   const SatelliteView& map,
                   size_t max_steps, size_t num_shells,
                   Player& player1, Player& player2,
                   TankAlgorithmFactory player1_tank_algo_factory,
                   TankAlgorithmFactory player2_tank_algo_factory) override;

private:
    void runGameLoop();
    bool readBoard(const std::string& filename);
    bool readBoard(size_t map_width, size_t map_height,
                   const SatelliteView& map,
                   size_t max_steps, size_t num_shells,
                   Player& player1, Player& player2,
                   TankAlgorithmFactory player1_tank_algo_factory,
                   TankAlgorithmFactory player2_tank_algo_factory);
    bool isGameOver() const;
    void doTanksStep();
    void getTanksActions();
    void checkActionsValidity();
    void handleTie();

    GameResult generateResult() const;
    void logTankActions();

    std::unique_ptr<Board> board_;
    std::vector<std::shared_ptr<Tank>> ordered_tanks_;
    size_t total_max_steps_;
    std::optional<std::size_t> tie_countdown_;
    size_t half_steps_count_ = 0;
    std::vector<bool> was_alive_at_round_start_;
    std::vector<std::optional<ActionRequest>> actions_to_execute_;
    std::vector<bool> actions_validity_;
    OutputLogger logger_;
    bool verbose_;
};
