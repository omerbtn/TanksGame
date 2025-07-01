#pragma once

#include <cassert>
#include <string>
#include <vector>

#include "Player.h"
#include "TankAlgorithm.h"


class AlgorithmRegistrar
{
    class AlgorithmAndPlayerFactories
    {
        std::string name_;
        TankAlgorithmFactory tank_algorithm_factory_;
        PlayerFactory player_factory_;

    public:
        AlgorithmAndPlayerFactories(const std::string& name) : name_(name) {}

        // Setters
        void setTankAlgorithmFactory(TankAlgorithmFactory&& factory)
        {
            assert(tank_algorithm_factory_ == nullptr && "Tank algorithm factory already set");
            tank_algorithm_factory_ = std::move(factory);
        }
        void setPlayerFactory(PlayerFactory&& factory)
        {
            assert(player_factory_ == nullptr && "Player factory already set");
            player_factory_ = std::move(factory);
        }

        // Getters
        const std::string& name() const { return name_; }
        TankAlgorithmFactory getTankAlgorithmFactory() const { return tank_algorithm_factory_; }

        // Creators
        std::unique_ptr<TankAlgorithm> createTankAlgorithm(int player_index, int tank_index) const
        {
            return tank_algorithm_factory_(player_index, tank_index);
        }
        std::unique_ptr<Player> createPlayer(int player_index, size_t x, size_t y, size_t max_steps, size_t num_shells) const
        {
            return player_factory_(player_index, x, y, max_steps, num_shells);
        }

        // Checkers
        bool hasTankAlgorithmFactory() const
        {
            return tank_algorithm_factory_ != nullptr;
        }
        bool hasPlayerFactory() const
        {
            return player_factory_ != nullptr;
        }
    };

    std::vector<AlgorithmAndPlayerFactories> algorithms_;
    static AlgorithmRegistrar registrar;

public:
    static AlgorithmRegistrar& getAlgorithmRegistrar();

    void createAlgorithmEntry(const std::string& name)
    {
        algorithms_.emplace_back(name);
    }
    void addTankAlgorithmFactoryToLastEntry(TankAlgorithmFactory&& factory)
    {
        algorithms_.back().setTankAlgorithmFactory(std::move(factory));
    }
    void addPlayerFactoryToLastEntry(PlayerFactory&& factory)
    {
        algorithms_.back().setPlayerFactory(std::move(factory));
    }

    // Validate registration
    struct BadRegistrationException
    {
        std::string name;
        bool has_name,
            has_tank_algorithm_factory,
            has_player_factory;
    };
    void validateLastRegistration() const
    {
        const auto& last = algorithms_.back();
        bool has_name = !last.name().empty();
        bool has_tank_algorithm_factory = last.hasTankAlgorithmFactory();
        bool has_player_factory = last.hasPlayerFactory();
        if (!has_name || !has_tank_algorithm_factory || !has_player_factory)
        {
            throw BadRegistrationException{
                .name = last.name(),
                .has_name = has_name,
                .has_tank_algorithm_factory = has_tank_algorithm_factory,
                .has_player_factory = has_player_factory};
        }
    }
    void removeLast()
    {
        algorithms_.pop_back();
    }

    // Iterators
    auto begin() { return algorithms_.begin(); }
    auto end() { return algorithms_.end(); }

    size_t count() const { return algorithms_.size(); }
    void clear() { algorithms_.clear(); }
};