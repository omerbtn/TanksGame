#pragma once

#include <cstddef>
#include <map>
#include <optional>

#include "TankAlgorithmFactory.h"
#include "PlayerFactory.h"
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
    GameManager(const PlayerFactory& playerFactory, const TankAlgorithmFactory& algorithmFactory)
        : playerFactory_(playerFactory), algorithmFactory_(algorithmFactory), board_(std::make_unique<Board>()) {}

    bool readBoard(const std::string& filename);
    void run();
    
private:
    static std::pair<std::string, std::string> split_filename(const std::string& filename);
    bool is_game_over() const;
    void do_tanks_step();

private:
    const PlayerFactory& playerFactory_;
    const TankAlgorithmFactory& algorithmFactory_;
    std::unique_ptr<Board> board_;
    std::unique_ptr<Player> player1_;  // Seems like players should be manager data and not board data, as manager is the one communicating with them
    std::unique_ptr<Player> player2_;  // Can switch to map of id to player but not important at the moment
    std::vector<std::pair<int, int>> all_tanks_;  // (player_id, tank_id) - Should execute actions in that order
    std::vector<std::unique_ptr<TankAlgorithm>> algorithms_;  // Ordered as all_tanks_
    size_t total_max_steps_;
    OutputLogger logger_;
    std::optional<std::size_t> tie_countdown_;
    size_t half_steps_count_ = 0;
};
