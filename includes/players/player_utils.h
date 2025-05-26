#pragma once

#include "cell.h"
#include "common/SatelliteView.h"

// For what? We already have the dimensions x and y from ctor
std::pair<size_t, size_t> get_dimensions_from_satellite(const SatelliteView& view) {
    size_t width = 0, height = 0;

    // Find width
    while (view.getObjectAt(width, 0) != '&') {
        ++width;
    }

    // Find height
    while (view.getObjectAt(0, height) != '&') {
        ++height;
    }

    return {width, height};
}

std::vector<std::vector<Cell>> reconstruct_grid_from_satellite_view(const SatelliteView& view, int player_index, Position& curr_tank_pos)
{
    auto [width, height] = get_dimensions_from_satellite(view);

    std::vector<std::vector<Cell>> grid(height, std::vector<Cell>(width));

    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            char ch = view.getObjectAt(x, y);
            Position pos{x, y};
            Cell cell(pos);

            switch (ch) {
                case '#':
                    cell.add_object(std::make_shared<Wall>());
                    break;
                case '@':
                    cell.add_object(std::make_shared<Mine>());
                    break;
                case '*':
                    cell.add_object(std::make_shared<Shell>(Direction::U));
                    break;
                case '1':
                    // TODO: later change that the battle info will hold all shells and tanks information.. - rethink
                    cell.add_object(std::make_shared<Tank>(1, 0, pos, Direction::L, 0));
                    break;
                case '2':
                    cell.add_object(std::make_shared<Tank>(2, 0, pos, Direction::R, 0));
                    break;
                case '%':
                    cell.add_object(std::make_shared<Tank>(player_index, 0, pos, Direction::L, 0));
                    curr_tank_pos = pos;
                    break;
                default:
                    break;
            }

            grid[x][y] = std::move(cell);
        }
    }

    return grid;
}
