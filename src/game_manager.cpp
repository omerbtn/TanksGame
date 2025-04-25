#include "game_manager.h"

#include <iostream>
#include "board.h"
#include "tank.h"
#include "player.h"
#include "output_logger.h"
#include "algorithms/algorithm_interface.h"
#include "algorithms/algorithm_utils.h"

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

    Player player1 = board_->players()[1];
    Player player2 = board_->players()[2];

    if (VERBOSE_DEBUG)
    {
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
    
    if (VERBOSE_DEBUG)            
        std::cout << "[GameManager] Player " << 1 << " decided to execute action: " << tankActionToString(action1) << std::endl;
    
    bool valid1 = board_->execute_tank_action(player1.tank(), action1);
    bool valid2 = board_->execute_tank_action(player2.tank(), action2);
    
    if (VERBOSE_DEBUG)
        std::cout << "[GameManager] Player " << 1 << " action " << (valid1 ? "succeeded" : "failed") << std::endl;

    logger.logAction(1, step_count_, action1, valid1);
    logger.logAction(2, step_count_, action2, valid2);

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
