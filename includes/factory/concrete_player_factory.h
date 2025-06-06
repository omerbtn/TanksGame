#pragma once

#include "PlayerFactory.h"
#include "concrete_player.h"
#include "smart_player.h"
#include "simple_player.h"


class ConcretePlayerFactory : public PlayerFactory
{
public:
    virtual ~ConcretePlayerFactory() = default;
    virtual std::unique_ptr<Player> create(int player_index, size_t x, size_t y, size_t max_steps, size_t num_shells) const override
    {
        if (player_index > 9 || player_index < 1)
        {
            throw std::invalid_argument("Invalid player index");
        }

        if (player_index % 2 == 0)
        {
            return std::make_unique<SimplePlayer>(player_index, x, y, max_steps, num_shells);
        }
        else
        {
            return std::make_unique<SmartPlayer>(player_index, x, y, max_steps, num_shells);
        }
    }
};
