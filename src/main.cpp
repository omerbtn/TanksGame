#include <iostream>

#include "game_manager.h"

#include "concrete_player_factory.h"
#include "concrete_tank_algorithm_factory.h"


int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: tanks_game <game_board_input_file>" << std::endl;
        return 1;
    }

    try
    {
        auto player_factory = ConcretePlayerFactory();
        auto algorithm_factory = ConcreteTankAlgorithmFactory();
        GameManager game{player_factory, algorithm_factory};
        game.readBoard(argv[1]);
        game.run();
    }
    catch (const std::exception& exc)
    {
        std::cout << "An exception was thrown: " << exc.what();
    }
    catch (...)
    {
        std::cout << "An unknown exception was thrown." << std::endl;
    }
    return 0;
}
