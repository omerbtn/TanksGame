#include "registrations/algorithm_registrar.h"


AlgorithmRegistrar AlgorithmRegistrar::registrar;

AlgorithmRegistrar& AlgorithmRegistrar::getAlgorithmRegistrar()
{
    return registrar;
}