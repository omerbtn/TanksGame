#include "registrations/algorithm_registrar.h"


namespace simulator
{
namespace registrations
{

AlgorithmRegistrar AlgorithmRegistrar::registrar;

AlgorithmRegistrar& AlgorithmRegistrar::getAlgorithmRegistrar()
{
    return registrar;
}

} // namespace registrations
} // namespace simulator