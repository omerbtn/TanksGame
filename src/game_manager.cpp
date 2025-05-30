#include "game_manager.h"

#include <iostream>
#include <algorithm>

#include "board.h"
#include "tank.h"
#include "player.h"
#include "output_logger.h"
#include "algorithms/algorithm_interface.h"
#include "algorithms/algorithm_utils.h"
#include "global_config.h"

GameManager::GameManager(const PlayerFactory& playerFactory, const TankAlgorithmFactory& algorithmFactory)
    : board_(std::make_unique<Board>(playerFactory, algorithmFactory)) {
}

std::pair<std::string, std::string> GameManager::split_filename(const std::string& filename)
{
    size_t last_slash_pos = filename.find_last_of("/\\");

    std::string directory;
    std::string name;

    if (last_slash_pos == std::string::npos)
    {
        // No directory component
        name = filename;
    }
    else
    {
        directory = filename.substr(0, last_slash_pos + 1); // Include the slash
        name = filename.substr(last_slash_pos + 1);
    }

    return std::make_pair(directory, name);
}

std::string GameManager::generate_result_message() const {
    std::unordered_map<int, int> alive_counts;

    for (const auto& tank : ordered_tanks_) {
        if (tank->is_alive()) {
            alive_counts[tank->player_id()]++;
        }
    }

    std::vector<std::pair<int, int>> players_alive;  // {player_id, alive_count}

    for (const auto& [player_id, count] : alive_counts) {
        players_alive.emplace_back(player_id, count);
    }

    std::string summary;

    if (players_alive.empty()) {
        summary = "Tie, all players have zero tanks";
    } else if (players_alive.size() == 1) {
        summary = "Player " + std::to_string(players_alive[0].first) + " won with " + std::to_string(players_alive[0].second) +
                  " tanks still alive";
    } else if (tie_countdown_.has_value() && *tie_countdown_ == 0) {
        summary = "Tie, both players have zero shells for " + std::to_string(config::get<int>("max_steps_after_tie")) + " steps";
    } else if (total_max_steps_ == 0) {
        summary = "Tie, reached max steps = " + std::to_string(half_steps_count_ / 2);
        std::map<int, int> full_counts;
        for (const auto& tank : ordered_tanks_) {
            full_counts[tank->player_id()] += tank->is_alive() ? 1 : 0;
        }

        for (const auto& [player_id, count] : full_counts) {
            summary += ", player " + std::to_string(player_id) + " has " + std::to_string(count) + " tanks";
        }
    }

    return summary;
}

bool GameManager::readBoard(const std::string& filename) {
    GameInfo game_info = board_->load_from_file(filename);

    if (!game_info.is_valid)
    {
        return false;
    }

    total_max_steps_ = game_info.max_steps;
    ordered_tanks_ = game_info.ordered_tanks;

    auto [directory, input_filename] = split_filename(filename);
    std::string output_filename = directory + static_cast<std::string>(config::get<std::string_view>("output_file_prefix")) + input_filename;

    logger_ = std::move(OutputLogger(output_filename, ordered_tanks_.size()));

    if (!logger_.is_valid()) {
        std::cerr << "Logger is invalid!\n";
        return false;
    }

    return true;
}

void GameManager::run()
{
    std::cout << "[GameManager] Starting game with the board:" << std::endl;
    board_->print();

    while (!is_game_over())
    {
        if (half_steps_count_ % 2 == 0)
        {
            std::cout << "[GameManager] Do tanks and shells step, half_steps_count = " << half_steps_count_ << std::endl;
            board_->do_shells_step(false);
            do_tanks_step();
        }
        else
        {
            std::cout << "[GameManager] Do shells step, half_steps_count = " << half_steps_count_ << std::endl;
            board_->do_shells_step(true);
        }
        board_->print();
        half_steps_count_++;
    }

    logger_.logResult(generate_result_message());
}

void GameManager::do_tanks_step()
{
    std::vector<std::optional<ActionRequest>> actions_to_execute;
    actions_to_execute.reserve(ordered_tanks_.size());

    // Get actions from all algorithms
    for (const auto& tank : ordered_tanks_) {
        if (!tank->is_alive()) {
            actions_to_execute.push_back(std::nullopt);
            continue;
        }

        const auto player_id = tank->player_id();
        const auto tank_id = tank->tank_id();
        auto algorithm = board_->get_algorithm(player_id, tank_id);
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
                      << " decided to execute action: " << tank_action_to_string(action_request) << std::endl;
        }

        actions_to_execute.push_back(action_request);
    }

    std::vector<bool> actions_validity;
    actions_validity.reserve(ordered_tanks_.size());

    // Execute actions and check validity
    for (size_t i = 0; i < ordered_tanks_.size(); ++i) 
    {
        const auto& tank = ordered_tanks_[i];
        auto action = actions_to_execute[i];
        
        if (!tank || !tank->is_alive() || !action)
        {
            actions_validity.push_back(false);
            continue;
        }
        
        bool valid = board_->execute_tank_action(tank, *action);
        actions_validity.push_back(valid);

        if constexpr (config::get<bool>("verbose_debug"))
        {
            std::cout << "[GameManager] Player " << tank->player_id() << " with tank " << tank->tank_id()
                        <<  " action " << tank_action_to_string(*action) << (valid ? " succeeded" : " failed") << std::endl;
        }
    
    }

    board_->update();

    // TODO: Check why not outputting (killed) for tank in the turn of death
    for (size_t i = 0; i < ordered_tanks_.size(); ++i) {
        logger_.logAction(i, actions_to_execute[i], actions_validity[i], ordered_tanks_[i]->is_alive());
    }

    if (total_max_steps_ > 0)
        --total_max_steps_;

    if (tie_countdown_.has_value())
    {
        if (*tie_countdown_ > 0)
            (*tie_countdown_)--;
    }
    else
    {
        // Handle the case all tanks used all their artillery
        bool all_tanks_out_of_ammo = true;

        for (int player_index = 1; player_index <= 9; ++player_index) {
            const auto& tanks = board_->get_player_tanks(player_index);

            for (const auto& tank : tanks) {
                if (tank->is_alive() && tank->ammo() > 0) {
                    all_tanks_out_of_ammo = false;
                    break;
                }
            }

            if (!all_tanks_out_of_ammo) {
                break;
            }
        }

        if (all_tanks_out_of_ammo) {
            tie_countdown_.emplace(config::get<int>("max_steps_after_tie"));
        }
    }
}

bool GameManager::is_game_over() const
{
    int alive_players = 0;

    for (int player_index = 1; player_index <= 9; ++player_index) {
        const auto& tanks = board_->get_player_tanks(player_index);
        if (std::any_of(tanks.begin(), tanks.end(), [](const auto& tank) { return tank->is_alive(); })) {
            ++alive_players;
        }
    }

    // Check if the game is over
    return alive_players <= 1 || total_max_steps_ == 0 || (tie_countdown_.has_value() && *tie_countdown_ == 0);
}
