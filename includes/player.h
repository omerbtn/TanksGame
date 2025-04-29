#pragma once

#include <memory>

#include "tank.h"
#include "algorithms/algorithm_interface.h"


class Player 
{
public:
    Player() = default;
    Player(std::shared_ptr<Tank> tank, std::shared_ptr<AlgorithmInterface> algorithm);
    Player(const Player& other) = default;
    Player& operator=(const Player& other) = default;
    Player(Player&& other) = default;
    Player& operator=(Player&& other) = default;

    std::shared_ptr<Tank> tank();
    const std::shared_ptr<Tank> tank() const;
    std::shared_ptr<AlgorithmInterface> algorithm();
    const std::shared_ptr<AlgorithmInterface> algorithm() const;

private:
    std::shared_ptr<Tank> tank_;
    std::shared_ptr<AlgorithmInterface> algorithm_;
};
