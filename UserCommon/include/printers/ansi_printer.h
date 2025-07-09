#pragma once

#include <iostream>

#include "printer.h"
#include "shell.h"
#include "tank.h"
#include "utils.h"


namespace UserCommon_322573304_322647603
{

constexpr const char* RESET = "\033[0m";
constexpr const char* RED = "\033[31m";
constexpr const char* GREEN = "\033[32m";
constexpr const char* YELLOW = "\033[33m";
constexpr const char* BLUE = "\033[34m";
constexpr const char* MAGENTA = "\033[35m";
constexpr const char* CYAN = "\033[36m";
constexpr const char* WHITE = "\033[37m";
constexpr const char* GRAY = "\033[90m";
constexpr const char* ORANGE = "\033[38;5;208m";
constexpr const char* PINK = "\033[38;5;13m";

class AnsiPrinter : public Printer<AnsiPrinter>
{
public:
    using Printer::Printer;

    void printImpl() const
    {
        std::cout << "Game Board:" << std::endl;

        for (size_t y = 0; y < height(); ++y)
        {
            for (size_t x = 0; x < width(); ++x)
            {
                const Cell& cell = grid()[x][y];
                std::string to_print = "[";

                // Walls
                if (cell.has(ObjectType::Wall))
                    to_print += std::string(GRAY) + "# " + RESET;

                // Mines
                if (cell.has(ObjectType::Mine))
                    to_print += std::string(RED) + "@ " + RESET;

                // Tanks
                if (cell.has(ObjectType::Tank))
                {
                    auto tank_obj = cell.getObjectByType(ObjectType::Tank); // Printing just one tank, couldn't be more
                    auto tank = std::static_pointer_cast<Tank>(tank_obj);
                    to_print += std::string(playerColor(tank->playerId())) + std::to_string(tank->playerId()) +
                                directionToArrow(tank->direction()) + RESET;
                }

                // Shells
                if (cell.has(ObjectType::Shell))
                {
                    auto shell_obj = cell.getObjectByType(ObjectType::Shell); // Printing just one shell, couldn't be more
                    auto shell = std::static_pointer_cast<Shell>(shell_obj);
                    to_print += std::string(YELLOW) + "*" + directionToArrow(shell->direction()) + RESET;
                }

                while (to_print.size() < 3)
                    to_print += " ";

                to_print += "]";
                std::cout << to_print;
            }

            std::cout << std::endl;
        }
    }

private:
    static const char* playerColor(int player_id)
    {
        switch (player_id)
        {
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

} // namespace UserCommon_322573304_322647603