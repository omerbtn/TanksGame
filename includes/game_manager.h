#pragma once

#include <cstddef>
#include <map>
#include <optional>

#include "Player.h"
#include "TankAlgorithm.h"
#include "board.h"
#include "tank.h"
#include "output_logger.h"
#include "game_info.h"
#include "ActionRequest.h"


class GameManager
{
public:
    GameManager(const PlayerFactory& playerFactory, const TankAlgorithmFactory& algorithmFactory);

    bool readBoard(const std::string& filename);
    void run();

private:
    static std::pair<std::string, std::string> splitFilename(const std::string& filename);
    bool isGameOver() const;
    void doTanksStep();
    std::string generateResultMessage() const;

    std::unique_ptr<Board> board_;
    std::vector<std::shared_ptr<Tank>> ordered_tanks_;
    size_t total_max_steps_;
    OutputLogger logger_;
    std::optional<std::size_t> tie_countdown_;
    size_t half_steps_count_ = 0;
};
