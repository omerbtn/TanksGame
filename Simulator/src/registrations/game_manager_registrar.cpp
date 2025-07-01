#include "registrations/game_manager_registrar.h"


GameManagerRegistrar GameManagerRegistrar::registrar;

GameManagerRegistrar& GameManagerRegistrar::getGameManagerRegistrar()
{
    return registrar;
}
