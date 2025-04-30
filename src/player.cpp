#include "player.h"

Player::Player(std::shared_ptr<Tank> tank, std::shared_ptr<AlgorithmInterface> algorithm)
    : tank_{tank}, algorithm_{algorithm} {}

std::shared_ptr<Tank> Player::tank() 
{
    return tank_;
}

const std::shared_ptr<Tank> Player::tank() const 
{
    return tank_;
}

std::shared_ptr<AlgorithmInterface> Player::algorithm() 
{
    return algorithm_;
}

const std::shared_ptr<AlgorithmInterface> Player::algorithm() const 
{
    return algorithm_;
}
