#include <iostream>

#include "game_manager.h"
#include "board.h"
#include "global_config.h"

int main(int argc, char* argv[]) {
    /*if (argc != 2)
    {
        std::cerr << "Usage: tanks_game <game_board_input_file>\n";
        return 1;
    }

    std::string file_path = argv[1];*/

    // Set the output stream to use UTF-8 for wide characters
    try {
        Board board;
        std::cout << "↑" << std::endl;

        if (!board.load_from_file(static_cast<std::string>(config::get<std::string_view>("map_file")))) {
            std::cout << "Failed to load game board.\n";
            return 1;
        }

        GameManager manager(&board);
        manager.run();
    } catch (const std::exception& exc) {
        std::cout << "An exception was thrown: " << exc.what();
    } catch (...) {
        std::cout << "An unknown exception was thrown.\n";
    }

    return 0;
}
