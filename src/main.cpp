#include <iostream>

#include "game_manager.h"
#include "board.h"
#include "global_config.h"

int main(int argc, char* argv[]) 
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
}
