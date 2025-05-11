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


//GameManager::GameManager(Board* board) : board_{board}, total_max_steps_(config::get<int>("total_max_steps")) {
//}

std::pair<std::string, std::string> GameManager::split_filename(const std::string& filename)
{
    size_t last_slash_pos = filename.find_last_of("/\\");

    std::string directory;
    std::string name;

    if (last_slash_pos == std::string::npos)
    {
        // No directory component
        directory = "";
        name = filename;
    }
    else
    {
        directory = filename.substr(0, last_slash_pos + 1); // Include the slash
        name = filename.substr(last_slash_pos + 1);
    }

    return std::make_pair(directory, name);
}

bool GameManager::readBoard(const std::string& filename) 
{
    GameInfo game_info = board_->load_from_file(filename);
    
    if (!game_info.is_valid) 
    {
        return false;
    }

    total_max_steps_ = game_info.max_steps;
    player1_ = playerFactory_.create(1, game_info.width, game_info.height, game_info.max_steps, game_info.num_shells);
    player2_ = playerFactory_.create(2, game_info.width, game_info.height, game_info.max_steps, game_info.num_shells);
    all_tanks_ = game_info.all_tanks;

    for (const auto& tank_info : all_tanks_) 
    {
        auto algorithm = algorithmFactory_.create(tank_info.first, tank_info.second);
        algorithms_.push_back(std::move(algorithm));
    }

    auto [directory, input_filename] = split_filename(filename);
    std::string output_filename = directory + static_cast<std::string>(config::get<std::string_view>("output_file_prefix")) + input_filename;

    logger_ = OutputLogger(output_filename);

    return true;
}

/*void GameManager::run()
{
    auto [directory, filename] = split_filename(board_->input_file_name());
    auto output_file = directory + static_cast<std::string>(config::get<std::string_view>("output_file_prefix")) + filename;
    OutputLogger logger(output_file);

    std ::cout << "[GameManager] Starting game with the board:" << std::endl;
    board_->print();

    while (!game_over())
    {
        board_->do_shells_step();
        if (half_steps_count_ % 2 == 0)
        {
            std::cout << "[GameManager] Do tanks step, half_steps_count = " << half_steps_count_ << std::endl;
            step(logger);
        }
        else
        {
            std::cout << "[GameManager] Do shells step, half_steps_count = " << half_steps_count_ << std::endl;
        }
        board_->print();
        half_steps_count_++;
    }

    logger.logResult(*board_->get_player_tank(1), *board_->get_player_tank(2), half_steps_count_ / 2);
}
*/    

void GameManager::run()
{
    std::cout << "[GameManager] Starting game with the board:" << std::endl;
    board_->print();

    while (!is_game_over())
    {
        board_->do_shells_step();
        if (half_steps_count_ % 2 == 0)
        {
            std::cout << "[GameManager] Do tanks and shells step, half_steps_count = " << half_steps_count_ << std::endl;
            do_tanks_step();
        }
        else
        {
            std::cout << "[GameManager] Do shells step, half_steps_count = " << half_steps_count_ << std::endl;
        }
        board_->print();
        half_steps_count_++;
    }

    // TODO: Output result to logger somehow
}

/*
void GameManager::step(OutputLogger& logger)
{
    Player player1 = board_->players()[1];
    Player player2 = board_->players()[2];

    if constexpr (config::get<bool>("verbose_debug"))
    {
        for (const auto& [id, player] : board_->players())
        {
            const auto& tank = *player.tank();
            std::cout << "[GameManager] Player " << id << " tank state: \n"
                      << "\tPosition: (" << tank.position().first << "," << tank.position().second << "), \n"
                      << "\tDirection: " << directionToString(tank.direction()) << ", \n"
                      << "\tAlive: " << (tank.is_alive() ? "yes" : "no") << ", \n"
                      << "\tAmmo: " << tank.ammo() << std::endl;
        }
    }

    TankAction action1 = player1.algorithm()->decideAction(*player1.tank(), *board_);
    TankAction action2 = player2.algorithm()->decideAction(*player2.tank(), *board_);

    if constexpr (config::get<bool>("verbose_debug"))
    {
        std::cout << "[GameManager] Player " << 1 << " decided to execute action: " << tank_action_to_string(action1) << std::endl;
        std::cout << "[GameManager] Player " << 2 << " decided to execute action: " << tank_action_to_string(action2) << std::endl;
    }

    bool valid1 = board_->execute_tank_action(player1.tank(), action1);
    bool valid2 = board_->execute_tank_action(player2.tank(), action2);

    if constexpr (config::get<bool>("verbose_debug"))
    {
        std::cout << "[GameManager] Player " << 1 << " action " << (valid1 ? "succeeded" : "failed") << std::endl;
        std::cout << "[GameManager] Player " << 2 << " action " << (valid2 ? "succeeded" : "failed") << std::endl;
    }

    board_->update();

    logger.logAction(1, half_steps_count_ / 2, action1, valid1);
    logger.logAction(2, half_steps_count_ / 2, action2, valid2);

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
        if (std::all_of(board_->players().begin(), board_->players().end(),
                        [](const auto& player) { return player.second.tank()->ammo() == 0; }))
        {
            tie_countdown_.emplace(config::get<int>("max_steps_after_tie"));
        }
    }
}

*/

void GameManager::do_tanks_step()
{
    std::vector<ActionRequest> actions_to_execute(all_tanks_.size());
    std::vector<bool> actions_validity(all_tanks_.size());

    // Get actions from all algorithms
    for (size_t i = 0; i < all_tanks_.size(); ++i)
    {
        const auto& tank_info = all_tanks_[i];
        ActionRequest action_request = algorithms_[i]->getAction();
        actions_to_execute[i] = action_request;

        if constexpr (config::get<bool>("verbose_debug"))
        {
            std::cout << "[GameManager] Player " << tank_info.first << "with tank " << tank_info.second
                      << " decided to execute action: " << tank_action_to_string(action_request) << std::endl;
        }
    }

    // TODO: Handle GetBattleInfo action

    // Execute actions and check validity
    for (size_t i = 0; i < all_tanks_.size(); ++i)
    {
        const auto& tank_info = all_tanks_[i];
        const auto& tank = board_->get_tank(tank_info.first, tank_info.second);
        if (tank && tank->is_alive())
        {
            bool valid = board_->execute_tank_action(tank, actions_to_execute[i]);  // TODO: Maybe turn this function to get tank info (id's) instead of pointer
            actions_validity[i] = valid;

            if constexpr (config::get<bool>("verbose_debug"))
            {
                std::cout << "[GameManager] Player " << tank_info.first << "with tank" << tank_info.second
                          <<  " action " << (valid ? "succeeded" : "failed") << std::endl;
            }
        }
    }

    board_->update();

    // TODO: Log actions somehow

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
        if (std::all_of(board_->get_player_tanks(1).begin(), board_->get_player_tanks(1).end(),
                        [](const auto& tank) { return tank->ammo() == 0; }) &&
            std::all_of(board_->get_player_tanks(2).begin(), board_->get_player_tanks(2).end(),
                        [](const auto& tank) { return tank->ammo() == 0; }))
        {
            tie_countdown_.emplace(config::get<int>("max_steps_after_tie"));
        }
    }
}

bool GameManager::is_game_over() const
{
    // Check if any tank is alive
    bool player1_alive = std::any_of(board_->get_player_tanks(1).begin(), board_->get_player_tanks(1).end(),
                                    [](const auto& tank) { return tank->is_alive(); });
    
    bool player2_alive = std::any_of(board_->get_player_tanks(2).begin(), board_->get_player_tanks(2).end(),
                                    [](const auto& tank) { return tank->is_alive(); });

    // Check if the game is over
    return !player1_alive || !player2_alive || total_max_steps_ == 0 || (tie_countdown_.has_value() && *tie_countdown_ == 0);
}