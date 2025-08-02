#include "registrations/game_manager_registrar.h"

namespace simulator
{
namespace registrations
{


GameManagerRegistrar GameManagerRegistrar::registrar;

GameManagerRegistrar& GameManagerRegistrar::getGameManagerRegistrar()
{
    return registrar;
}

} // namespace registrations
} // namespace simulator