#pragma once

#include "PlayerFactory.h"
#include "concrete_player.h"
#include "smart_player.h"

#define PLAYER_TYPES_XMACRO \
    X(1)                    \
    X(2)                    \
    X(3)                    \
    X(4)                    \
    X(5)                    \
    X(6)                    \
    X(7)                    \
    X(8)                    \
    X(9)

class ConcretePlayerFactory : public PlayerFactory
{
public:
    virtual ~ConcretePlayerFactory() = default;
    virtual std::unique_ptr<Player> create(int player_index, size_t x, size_t y, size_t max_steps, size_t num_shells) const override
    {
        /*
        // Added X-MACRO to avoid code duplication
        switch (player_index) {
#define X(N) \
    case N:  \
        return std::make_unique<ConcretePlayer<N>>(x, y, max_steps, num_shells);
            PLAYER_TYPES_XMACRO
#undef X
            default:
                throw std::invalid_argument("Invalid player index");
        }
        */

        // TODO: Create SimplePlayer
        return std::make_unique<SmartPlayer>(player_index, x, y, max_steps, num_shells);
    }
};
