#include <iostream>

#include "game_manager.h"
#include "board.h"

int main(int argc, char *argv[])
{
    /*if (argc != 2)
    {
        std::cerr << "Usage: tanks_game <game_board_input_file>\n";
        return 1;
    }

    std::string file_path = argv[1];*/

    // Set the output stream to use UTF-8 for wide characters

    Board board;
    std::cout << "↑" << std::endl;

    if (!board.load_from_file("resources/game_map.txt"))
    {
        std::cout << "Failed to load game board.\n";
        return 1;
    }

    std::shared_ptr<Tank> tank1 = board.get_player_tank(1);
    std::shared_ptr<Tank> tank2 = board.get_player_tank(2);

    if (!tank1 || !tank2)
    {
        std::cerr << "Missing one or both tanks.\n";
        return 1;
    }

    GameManager manager(&board);
    manager.run();

    return 0;
}
