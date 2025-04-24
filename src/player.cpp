#include "player.h"

Player::Player(std::shared_ptr<Tank> tank, std::shared_ptr<AlgorithmInterface> algorithm)
    : tank_{tank}, algorithm_{algorithm} {
}

Tank* Player::tank() {
    return tank_.get();
}

const Tank* Player::tank() const {
    return tank_.get();
}

AlgorithmInterface* Player::algorithm() {
    return algorithm_.get();
}

const AlgorithmInterface* Player::algorithm() const {
    return algorithm_.get();
}
