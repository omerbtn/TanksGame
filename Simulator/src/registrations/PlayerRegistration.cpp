#include "PlayerRegistration.h"

#include "registrations/algorithm_registrar.h"


PlayerRegistration::PlayerRegistration(PlayerFactory factory)
{
    auto& registrar = AlgorithmRegistrar::getAlgorithmRegistrar();
    registrar.addPlayerFactoryToLastEntry(std::move(factory));
}