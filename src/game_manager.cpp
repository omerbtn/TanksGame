#include "game_manager.h"

#include "board.h"
#include "tank.h"
#include "player.h"
#include "output_logger.h"
#include "algorithms/algorithm_interface.h"

GameManager::GameManager(Board *board) : board_(board), step_count_(0), tie_countdown_(40) {
}

void GameManager::run() {
    OutputLogger logger("output.txt");  // TODO move to the static config
    while (!game_over()) {
        step(logger);
        board_->print();
    }

    logger.logResult(*board_->get_player_tank(1), *board_->get_player_tank(2), step_count_);
}

void GameManager::step(OutputLogger& logger) {
    board_->update();

    for (auto& [id, player]: board_->players()) {
        TankAction action = player.algorithm()->decideAction(*player.tank(), *board_);

        bool valid = board_->execute_tank_action(player.tank(), action);

        logger.logAction(id, step_count_, action, valid);
    }

    ++step_count_;
    // TODO: add countdown for tie and and another general one to avoid infinite loop
    if (tie_countdown_ > 0) --tie_countdown_;
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
    if (tie_countdown_ == 0) {
        return true;
    }

    return false;
}
