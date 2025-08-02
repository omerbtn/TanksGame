#include "GameManagerRegistration.h"

#include "registrations/game_manager_registrar.h"

GameManagerRegistration::GameManagerRegistration(GameManagerFactory factory)
{
    auto& registrar = simulator::registrations::GameManagerRegistrar::getGameManagerRegistrar();
    registrar.addGameManagerFactoryToLastEntry(std::move(factory));
}