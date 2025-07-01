#pragma once

#include <cassert>
#include <string>
#include <vector>

#include "AbstractGameManager.h"


class GameManagerRegistrar
{
    class GameManagerFactoryEntry
    {
        std::string name_;
        GameManagerFactory game_manager_factory_;
    public:
        GameManagerFactoryEntry(const std::string& name) : name_(name) {}

        // Setter
        void setGameManagerFactory(GameManagerFactory&& factory)
        {
            assert(game_manager_factory_ == nullptr && "Game manager factory already set");
            game_manager_factory_ = std::move(factory);
        }

        const std::string& name() const { return name_; }

        // Create
        std::unique_ptr<AbstractGameManager> createGameManager(bool verbose) const
        {
            return game_manager_factory_(verbose);
        }

        // Checker
        bool hasGameManagerFactory() const
        {
            return game_manager_factory_ != nullptr;
        }
    };

    std::vector<GameManagerFactoryEntry> managers_;
    static GameManagerRegistrar registrar;

public:
    static GameManagerRegistrar& getGameManagerRegistrar();

    void createGameManagerEntry(const std::string& name)
    {
        managers_.emplace_back(name);
    }
    void addGameManagerFactoryToLastEntry(GameManagerFactory&& factory)
    {
        managers_.back().setGameManagerFactory(std::move(factory));
    }

    // Validate registration
    struct BadRegistrationException
    {
        std::string name;
        bool has_name, has_game_manager_factory;
    };
    void validateLastRegistration() const
    {
        const auto& last = managers_.back();
        bool has_name = !last.name().empty();
        bool has_game_manager_factory = last.hasGameManagerFactory();
        if (!has_name || !has_game_manager_factory)
        {
            throw BadRegistrationException{
                .name = last.name(),
                .has_name = has_name,
                .has_game_manager_factory = has_game_manager_factory
            };
        }
    }
    void removeLast()
    {
        managers_.pop_back();
    }

    // Iterators
    auto begin() { return managers_.begin(); }
    auto end() { return managers_.end(); }

    size_t count() const { return managers_.size(); }
    void clear() { managers_.clear(); }
};
