#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

#include "algorithm_registrar.h"
#include "game_manager_registrar.h"


template <typename Registrar>
struct RegistrarAdapter; // Intentionally left undefined to enforce specialization

// Specialization for GameManagerRegistrar
template <>
struct RegistrarAdapter<GameManagerRegistrar>
{
    static void createEntry(const std::string& name)
    {
        GameManagerRegistrar::getGameManagerRegistrar().createGameManagerEntry(name);
    }

    static void validateLast()
    {
        GameManagerRegistrar::getGameManagerRegistrar().validateLastRegistration();
    }

    static void removeLast()
    {
        GameManagerRegistrar::getGameManagerRegistrar().removeLast();
    }

    using BadRegistrationException = GameManagerRegistrar::BadRegistrationException;

    static std::string getBadRegistrationDetails(const BadRegistrationException& e)
    {
        std::ostringstream oss;
        oss << "Bad GameManager registration:" << '\n'
            << "  Name: " << e.name << '\n'
            << "  Has name? " << std::boolalpha << e.has_name << '\n'
            << "  Has game manager factory? " << std::boolalpha << e.has_game_manager_factory << '\n';

        return oss.str();
    }
};

// Specialization for AlgorithmRegistrar
template <>
struct RegistrarAdapter<AlgorithmRegistrar>
{
    static void createEntry(const std::string& name)
    {
        AlgorithmRegistrar::getAlgorithmRegistrar().createAlgorithmEntry(name);
    }

    static void validateLast()
    {
        AlgorithmRegistrar::getAlgorithmRegistrar().validateLastRegistration();
    }

    static void removeLast()
    {
        AlgorithmRegistrar::getAlgorithmRegistrar().removeLast();
    }

    using BadRegistrationException = AlgorithmRegistrar::BadRegistrationException;

    static std::string getBadRegistrationDetails(const BadRegistrationException& e)
    {
        std::ostringstream oss;
        oss << "Bad Algorithm registration:" << '\n'
            << "  Name: " << e.name << '\n'
            << "  Has name? " << std::boolalpha << e.has_name << '\n'
            << "  Has tank algorithm factory? " << std::boolalpha << e.has_tank_algorithm_factory << '\n'
            << "  Has player factory? " << std::boolalpha << e.has_player_factory << '\n';

        return oss.str();
    }
};
