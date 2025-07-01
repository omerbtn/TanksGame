#include "game_manager.h"

#include <algorithm>
#include <iostream>

#include "GameManagerRegistration.h"
#include "Player.h"
#include "board.h"
#include "board_satellite_view.h"
#include "global_config.h"
#include "output_logger.h"
#include "tank.h"
#include "utils.h"

REGISTER_GAME_MANAGER(GameManager)


GameManager::GameManager(bool verbose) : board_(std::make_unique<Board>()), verbose_(verbose) {}

GameResult GameManager::generateResult() const
{
    std::map<int, size_t> alive_counts;
    GameResult game_result;

    for (const auto& tank : ordered_tanks_)
    {
        alive_counts[tank->playerId()] += tank->isAlive() ? 1 : 0;
    }

    std::vector<std::pair<int, size_t>> players_alive; // {player_id, alive_count}

    for (const auto& [player_id, count] : alive_counts)
    {
        if (count > 0)
        {
            players_alive.emplace_back(player_id, count);
        }
        game_result.remaining_tanks.push_back(count);
    }

    // Check for tie
    if (players_alive.empty() ||
        (tie_countdown_ && *tie_countdown_ == 0) ||
        total_max_steps_ == 0)
    {
        game_result.winner = 0; // Tie

        if (players_alive.empty())
        {
            game_result.reason = GameResult::Reason::ALL_TANKS_DEAD;
        }
        else if (tie_countdown_ && *tie_countdown_ == 0)
        {
            game_result.reason = GameResult::Reason::ZERO_SHELLS;
        }
        else
        {
            game_result.reason = GameResult::Reason::MAX_STEPS;
        }
    }
    else if (players_alive.size() == 1)
    {
        // We have a winner
        game_result.winner = players_alive[0].first;
        game_result.reason = GameResult::Reason::ALL_TANKS_DEAD;
    }

    game_result.rounds = half_steps_count_ / 2;

    game_result.game_state = std::make_unique<BoardSatelliteView>(board_->getGrid());

    return game_result;
}

// bool GameManager::readBoard(const std::string& filename)
// {
//     GameInfo game_info = board_->loadFromFile(filename);

//     if (!game_info.is_valid)
//     {
//         return false;
//     }

//     total_max_steps_ = game_info.max_steps;
//     ordered_tanks_ = game_info.ordered_tanks;

//     auto [directory, input_filename] = splitFilename(filename);
//     std::string output_filename = directory + static_cast<std::string>(config::get<std::string_view>("output_file_prefix")) + input_filename;

//     logger_ = OutputLogger(output_filename, ordered_tanks_.size());

//     if (!logger_.is_valid())
//     {
//         std::cerr << "Logger is invalid!" << std::endl;
//         return false;
//     }

//     return true;
// }

bool GameManager::readBoard(size_t map_width, size_t map_height,
                            const SatelliteView& map,
                            size_t max_steps, size_t num_shells,
                            Player& player1, Player& player2,
                            TankAlgorithmFactory player1_tank_algo_factory,
                            TankAlgorithmFactory player2_tank_algo_factory)
{
    GameInfo game_info = board_->loadFromSatelliteView(map_width, map_height, map,
                                                       max_steps, num_shells,
                                                       player1, player2,
                                                       player1_tank_algo_factory,
                                                       player2_tank_algo_factory);

    if (!game_info.is_valid)
    {
        return false;
    }

    total_max_steps_ = game_info.max_steps;
    ordered_tanks_ = std::move(game_info.ordered_tanks);

    if (verbose_)
    {
        std::string output_filename = std::string(config::get<std::string_view>("output_file_prefix")) + getUniqueTimeString() + ".log";
        logger_ = OutputLogger(output_filename, ordered_tanks_.size());

        if (!logger_.is_valid())
        {
            std::cerr << "Logger is invalid!" << std::endl;
            return false;
        }
    }

    return true;
}

void GameManager::logTankActions()
{
    // Capture final alive status (after all actions and updates)
    std::vector<bool> is_alive_at_end;
    is_alive_at_end.reserve(ordered_tanks_.size());
    for (const auto& tank : ordered_tanks_)
    {
        is_alive_at_end.push_back(tank->isAlive());
    }
    // Log actions with proper death detection
    for (size_t i = 0; i < ordered_tanks_.size(); ++i)
    {
        bool died_this_round = was_alive_at_round_start_[i] && !is_alive_at_end[i];
        logger_.logAction(i, actions_to_execute_[i], actions_validity_[i],
                          was_alive_at_round_start_[i], died_this_round);
    }
}

GameResult GameManager::run(size_t map_width, size_t map_height,
                            const SatelliteView& map,
                            size_t max_steps, size_t num_shells,
                            Player& player1, Player& player2,
                            TankAlgorithmFactory player1_tank_algo_factory,
                            TankAlgorithmFactory player2_tank_algo_factory)
{
    if (!readBoard(map_width, map_height, map, max_steps, num_shells,
                   player1, player2,
                   player1_tank_algo_factory, player2_tank_algo_factory))
    {
        std::cerr << "[GameManager] Failed to read board from satellite view." << std::endl;
        return GameResult{0, GameResult::Reason::ALL_TANKS_DEAD, {}, nullptr, 0};
    }

    runGameLoop();

    GameResult result = generateResult();

    logger_.logResult(resultToString(result));

    return result;
}

void GameManager::runGameLoop()
{
    std::cout << "[GameManager] Starting game with the board:" << std::endl;
    board_->print();

    was_alive_at_round_start_.reserve(ordered_tanks_.size());
    for (const auto& tank : ordered_tanks_)
    {
        was_alive_at_round_start_.push_back(tank->isAlive());
    }

    while (true)
    {
        if (half_steps_count_ % 2 == 0)
        {
            std::cout << "[GameManager] Do tanks and shells step, half_steps_count = " << half_steps_count_ << std::endl;

            for (size_t i = 0; i < ordered_tanks_.size(); ++i)
            {
                was_alive_at_round_start_[i] = ordered_tanks_[i]->isAlive();
            }

            doTanksStep();
            board_->print();

            board_->doShellsStep(false);
            board_->print();
        }
        else
        {
            std::cout << "[GameManager] Do shells step, half_steps_count = " << half_steps_count_ << std::endl;

            board_->doShellsStep(true);

            logTankActions();

            board_->print();

            if (isGameOver())
            {
                break;
            }
        }
        half_steps_count_++;
    }
}

void GameManager::getTanksActions()
{
    std::vector<std::optional<ActionRequest>> actions_to_execute;
    actions_to_execute.reserve(ordered_tanks_.size());

    // Get actions from all algorithms
    for (const auto& tank : ordered_tanks_)
    {
        if (!tank->isAlive())
        {
            actions_to_execute.push_back(std::nullopt);
            continue;
        }

        const auto player_id = tank->playerId();
        const auto tank_id = tank->tankId();
        auto algorithm = board_->getAlgorithm(player_id, tank_id);
        if (!algorithm)
        {
            std::cerr << "[GameManager] Algorithm not found for player " << player_id << " with tank " << tank_id << std::endl;
            actions_to_execute.push_back(std::nullopt);
            continue;
        }

        ActionRequest action_request = algorithm->getAction();

        if constexpr (config::get<bool>("verbose_debug"))
        {
            std::cout << "[GameManager] Player " << player_id << " with tank " << tank_id
                      << " decided to execute action: " << tankActionToString(action_request) << std::endl;
        }

        actions_to_execute.push_back(action_request);
    }

    actions_to_execute_ = std::move(actions_to_execute);
}

void GameManager::checkActionsValidity()
{
    std::vector<bool> actions_validity;
    actions_validity.reserve(ordered_tanks_.size());

    // Execute actions and check validity
    for (size_t i = 0; i < ordered_tanks_.size(); ++i)
    {
        const auto& tank = ordered_tanks_[i];
        auto action = actions_to_execute_[i];

        if (!tank || !tank->isAlive() || !action)
        {
            actions_validity.push_back(false);
            continue;
        }

        bool valid = board_->executeTankAction(tank, *action);
        actions_validity.push_back(valid);

        tank->setLastAction(*action);

        if constexpr (config::get<bool>("verbose_debug"))
        {
            std::cout << "[GameManager] Player " << tank->playerId() << " with tank " << tank->tankId()
                      << " action " << tankActionToString(*action) << (valid ? " succeeded" : " failed") << std::endl;
        }
    }

    actions_validity_ = std::move(actions_validity);
}

void GameManager::handleTie()
{
    if (tie_countdown_.has_value())
    {
        if (*tie_countdown_ > 0)
            (*tie_countdown_)--;
    }
    else
    {
        // Handle the case all tanks used all their artillery
        bool all_tanks_out_of_ammo = true;

        for (int player_index = 1; player_index <= 9; ++player_index)
        {
            const auto& tanks = board_->getPlayerTanks(player_index);

            for (const auto& tank : tanks)
            {
                if (tank->isAlive() && tank->ammo() > 0)
                {
                    all_tanks_out_of_ammo = false;
                    break;
                }
            }

            if (!all_tanks_out_of_ammo)
            {
                break;
            }
        }

        if (all_tanks_out_of_ammo)
        {
            tie_countdown_.emplace(config::get<int>("max_steps_after_tie"));
        }
    }
}

void GameManager::doTanksStep()
{
    getTanksActions();

    checkActionsValidity();

    board_->update();

    if (total_max_steps_ > 0)
        --total_max_steps_;

    handleTie();
}

bool GameManager::isGameOver() const
{
    int alive_players = 0;

    for (int player_index = 1; player_index <= 9; ++player_index)
    {
        const auto& tanks = board_->getPlayerTanks(player_index);
        if (std::any_of(tanks.begin(), tanks.end(), [](const auto& tank)
                        { return tank->isAlive(); }))
        {
            ++alive_players;
        }
    }

    // Check if the game is over
    return alive_players <= 1 || total_max_steps_ == 0 || (tie_countdown_.has_value() && *tie_countdown_ == 0);
}
