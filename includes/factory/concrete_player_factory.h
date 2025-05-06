#pragma once

#include "common/PlayerFactory.h"

#include "players/concrete_player_1.h"
#include "players/concrete_player_2.h"

class ConcretePlayerFactory : public PlayerFactory
{
public:
    virtual ~ConcretePlayerFactory() = default;
    virtual std::unique_ptr<Player> create(int player_index, size_t x, size_t y, size_t max_steps, size_t num_shells) const override
    {
        if (player_index == 1)
        {
            return std::make_unique<ConcretePlayer1>(player_index, x, y, max_steps, num_shells);
        }
        else if (player_index == 2)
        {
            return std::make_unique<ConcretePlayer2>(player_index, x, y, max_steps, num_shells);
        }
        else
        {
            throw std::invalid_argument("Invalid player index");
        }
    }
};
