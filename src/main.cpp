#include "game_manager.h"
#include "board.h"

#include "algorithms/aggressive_chase_algorithm.h"
#include "algorithms/smart_algorithm.h"

int main(int argc, char *argv[])
{
    /*if (argc != 2)
    {
        std::cerr << "Usage: tanks_game <game_board_input_file>\n";
        return 1;
    }

    std::string file_path = argv[1];*/
    Board board;

    if (!board.loadFromFile("resources/game_map.txt"))
    {
        std::cout << "Failed to load game board.\n";
        return 1;
    }

    Tank *tank1 = board.getPlayerTank(1);
    Tank *tank2 = board.getPlayerTank(2);

    if (!tank1 || !tank2)
    {
        std::cerr << "Missing one or both tanks.\n";
        return 1;
    }

    AlgorithmInterface *algo1 = new SmartAlgorithm();
    AlgorithmInterface *algo2 = new AggressiveChaseAlgorithm();

    GameManager manager(&board, tank1, tank2, algo1, algo2);
    manager.run();

    delete algo1;
    delete algo2;
    return 0;
}
