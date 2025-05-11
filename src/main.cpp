#include <iostream>

#include "game_manager.h"
#include "board.h"
#include "global_config.h"

#include "concrete_player_factory.h"
#include "concrete_tank_algorithm_factory.h"

int main(int argc, char** argv) 
{
    if (argc != 2)
    {
        std::cerr << "Usage: tanks_game <game_board_input_file>" << std::endl;
        return 1;
    }

    // There was a problem with the temporary factories, they died before readBoard, so moved to create them here
    // Seems ok with the assignment requirements
    ConcretePlayerFactory playerFactory;
    ConcreteTankAlgorithmFactory algorithmFactory;

    GameManager game{playerFactory, algorithmFactory};
    game.readBoard(argv[1]);
    game.run();

    return 0;
}

/*int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: tanks_game <game_board_input_file>" << std::endl;
        return 1;
    }

    std::string file_path = argv[1];

    try {
        Board board;

        if (!board.load_from_file(file_path)) {
            std::cout << "Failed to load game board." << std::endl;
            return 1;
        }

        GameManager manager(&board);
        manager.run();
    } catch (const std::exception& exc) {
        std::cout << "An exception was thrown: " << exc.what();
    } catch (...) {
        std::cout << "An unknown exception was thrown." << std::endl;
    }

    return 0;
}*/
