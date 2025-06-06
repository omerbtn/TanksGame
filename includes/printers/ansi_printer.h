#pragma once

#include <iostream>

#include "printer.h"
#include "algorithm_utils.h"

#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"
#define GRAY "\033[90m"
#define ORANGE "\033[38;5;208m"
#define PINK "\033[38;5;13m"

class AnsiPrinter : public Printer<AnsiPrinter> {
public:
    using Printer::Printer;

    void printImpl() const {
        std::cout << "Game Board:" << std::endl;

        for (size_t y = 0; y < height(); ++y) {
            for (size_t x = 0; x < width(); ++x) {
                const Cell& cell = grid()[x][y];
                std::string to_print = "[";

                // Walls
                if (cell.has(ObjectType::Wall)) to_print += std::string(GRAY) + "# " + RESET;

                // Mines
                if (cell.has(ObjectType::Mine)) to_print += std::string(RED) + "@ " + RESET;

                // Tanks
                if (cell.has(ObjectType::Tank)) {
                    const auto& tanks = cell.getObjectsByType(ObjectType::Tank);
                    if (!tanks.empty()) {
                        auto tank = std::static_pointer_cast<Tank>(tanks.front());  // Printing just one tank, couldn't be more
                        to_print += std::string(playerColor(tank->playerId())) + std::to_string(tank->playerId()) +
                                    directionToArrow(tank->direction()) + RESET;
                    }
                }

                // Shells
                if (cell.has(ObjectType::Shell)) {
                    const auto& shells = cell.getObjectsByType(ObjectType::Shell);
                    if (!shells.empty()) {
                        auto shell = std::static_pointer_cast<Shell>(shells.front());  // Printing just one shell, couldn't be more
                        to_print += std::string(YELLOW) + "*" + directionToArrow(shell->direction()) + RESET;
                    }
                }

                while (to_print.size() < 3) to_print += " ";

                to_print += "]";
                std::cout << to_print;
            }

            std::cout << std::endl;
        }
    }

private:
    static const char* playerColor(int player_id) {
        switch (player_id) {
            case 1:
                return GREEN;
            case 2:
                return BLUE;
            case 3:
                return CYAN;
            case 4:
                return MAGENTA;
            case 5:
                return YELLOW;
            case 6:
                return WHITE;
            case 7:
                return RED;
            case 8:
                return ORANGE;
            case 9:
                return PINK;
            default:
                return WHITE;
        }
    }
};
