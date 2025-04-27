#include "game_manager.h"

#include <iostream>

#include "board.h"
#include "tank.h"
#include "player.h"
#include "output_logger.h"
#include "algorithms/algorithm_interface.h"
#include "algorithms/algorithm_utils.h"
#include "global_config.h"

GameManager::GameManager(Board* board) : board_{board}, total_max_steps_(config::get<int>("total_max_steps")) {
}

static std::pair<std::string, std::string> split_filename(const std::string& filename) {
    size_t dot_pos = filename.rfind('.');

    if (dot_pos == std::string::npos) {
        return std::make_pair(filename, "");
    }

    std::string name = filename.substr(0, dot_pos);
    std::string extension = filename.substr(dot_pos + 1);

    return std::make_pair(name, extension);
}

void GameManager::run() {
    auto [filename, ext] = split_filename(board_->input_file_name());
    auto output_file = filename + static_cast<std::string>(config::get<std::string_view>("output_file_suffix")) + ext;
    OutputLogger logger(output_file);
    while (!game_over()) {
        step(logger);
        board_->print();
    }

    logger.logResult(*board_->get_player_tank(1), *board_->get_player_tank(2), step_count_);
}

void GameManager::step(OutputLogger& logger) {
    board_->update();

    Player player1 = board_->players()[1];
    Player player2 = board_->players()[2];

    if constexpr (config::get<bool>("verbose_debug")) {
        // TODO: Fix
        Tank tank = *player1.tank();
        std::cout << "[GameManager] Player " << 1 << " tank state: \n"
                    << "\tPosition: (" << tank.position().first << "," << tank.position().second << "), \n"
                    << "\tDirection: " << directionToString(tank.direction()) << ", \n"
                    << "\tAlive: " << (tank.is_alive() ? "yes" : "no") << ", \n"
                    << "\tAmmo: " << tank.ammo() << std::endl;
    }
    TankAction action1 = player1.algorithm()->decideAction(*player1.tank(), *board_);
    TankAction action2 = player2.algorithm()->decideAction(*player2.tank(), *board_);

    if constexpr (config::get<bool>("verbose_debug")) {
        std::cout << "[GameManager] Player " << 1 << " decided to execute action: " << tank_action_to_string(action1) << std::endl;
    }

    bool valid1 = board_->execute_tank_action(player1.tank(), action1);
    bool valid2 = board_->execute_tank_action(player2.tank(), action2);

    if constexpr (config::get<bool>("verbose_debug")) {
        std::cout << "[GameManager] Player " << 1 << " action " << (valid1 ? "succeeded" : "failed") << std::endl;
    }

    logger.logAction(1, step_count_, action1, valid1);
    logger.logAction(2, step_count_, action2, valid2);

    ++step_count_;
    if (total_max_steps_ > 0) {
        --total_max_steps_;
    }

    if (tie_countdown_.has_value()) {
        if (*tie_countdown_ > 0) {
            (*tie_countdown_)--;
        }
    } else {
        // Handle the case all tanks used all their artillery
        if (std::all_of(board_->players().begin(), board_->players().end(),
                        [](const auto& player) { return player.second.tank()->ammo() == 0; })) {
            tie_countdown_.emplace(config::get<int>("max_steps_after_tie"));
        }
    }
}

bool GameManager::game_over() const {
    const auto tank1 = board_->get_player_tank(1);
    const auto tank2 = board_->get_player_tank(2);

    if (!tank1->is_alive() && !tank2->is_alive()) {
        return true;
    }
    if (!tank1->is_alive() || !tank2->is_alive()) {
        return true;
    }
    if (total_max_steps_ == 0 || (tie_countdown_.has_value() && *tie_countdown_ == 0)) {
        return true;
    }

    return false;
}
