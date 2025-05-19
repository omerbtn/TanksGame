#pragma once

#include <iostream>

#include "printer.h"
#include "algorithm_utils.h"

class DefaultPrinter : public Printer<DefaultPrinter> {
public:
    using Printer::Printer;

    void print_impl() const {
        std::cout << "Game Board:" << std::endl;

        for (size_t y = 0; y < height(); ++y) {
            for (size_t x = 0; x < width(); ++x) {
                const Cell& cell = grid()[x][y];
                std::string to_print = "[";

                // Walls
                if (cell.has(ObjectType::Wall)) to_print += "#";

                // Mines
                if (cell.has(ObjectType::Mine)) to_print += "@";

                // Tanks
                if (cell.has(ObjectType::Tank)) {
                    const auto& tanks = cell.get_objects_by_type(ObjectType::Tank);
                    if (!tanks.empty()) {
                        auto tank = std::static_pointer_cast<Tank>(tanks.front());  // Printing just one tank, couldn't be more
                        to_print += std::to_string(tank->player_id());
                        to_print += directionToArrow(tank->direction());
                    }
                }

                // Shells
                if (cell.has(ObjectType::Shell)) {
                    const auto& shells = cell.get_objects_by_type(ObjectType::Shell);
                    if (!shells.empty()) {
                        auto shell = std::static_pointer_cast<Shell>(shells.front());  // Printing just one shell, couldn't be more
                        to_print += "*";
                        to_print += directionToArrow(shell->direction());
                    }
                }

                while (to_print.size() < 3) to_print += " ";

                to_print += "]";
                std::cout << to_print;
            }

            std::cout << std::endl;
        }
    }
};
