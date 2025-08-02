#include "TankAlgorithmRegistration.h"

#include "registrations/algorithm_registrar.h"


TankAlgorithmRegistration::TankAlgorithmRegistration(TankAlgorithmFactory factory)
{
    auto& registrar = simulator::registrations::AlgorithmRegistrar::getAlgorithmRegistrar();
    registrar.addTankAlgorithmFactoryToLastEntry(std::move(factory));
}