#pragma once

#include <iostream>
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

    static void printBadRegistrationDetails(const BadRegistrationException& e)
    {
        std::cerr << "---------------------------------" << std::endl;
        std::cerr << "Bad GameManager registration:" << std::endl;
        std::cerr << "  Name: " << e.name << std::endl;
        std::cerr << "  Has name? " << std::boolalpha << e.has_name << std::endl;
        std::cerr << "  Has game manager factory? " << std::boolalpha << e.has_game_manager_factory << std::endl;
        std::cerr << "---------------------------------" << std::endl;
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

    static void printBadRegistrationDetails(const BadRegistrationException& e)
    {
        std::cerr << "---------------------------------" << std::endl;
        std::cerr << "Bad Algorithm registration:" << std::endl;
        std::cerr << "  Name: " << e.name << std::endl;
        std::cerr << "  Has name? " << std::boolalpha << e.has_name << std::endl;
        std::cerr << "  Has tank algorithm factory? " << std::boolalpha << e.has_tank_algorithm_factory << std::endl;
        std::cerr << "  Has player factory? " << std::boolalpha << e.has_player_factory << std::endl;
        std::cerr << "---------------------------------" << std::endl;
    }
};
