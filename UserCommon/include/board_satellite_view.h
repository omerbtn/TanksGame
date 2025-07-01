#pragma once

#include <vector>

#include "SatelliteView.h"
#include "cell.h"


class BoardSatelliteView : public SatelliteView
{
public:
    virtual ~BoardSatelliteView() = default;
    BoardSatelliteView(const std::vector<std::vector<Cell>>& grid, const Position tank_position)
        : tank_position_{tank_position}, for_tank_(true)
    {
        convertGridToCharsGrid(grid);
    }

    explicit BoardSatelliteView(const std::vector<std::vector<Cell>>& grid)
        : for_tank_(false)
    {
        convertGridToCharsGrid(grid);
    }

    explicit BoardSatelliteView(std::vector<std::vector<char>>&& chars_grid)
        : chars_grid_(std::move(chars_grid)), for_tank_(false) {}

    char getObjectAt(size_t x, size_t y) const override
    {
        if (outOfBounds(x, y))
            return '&';

        if (for_tank_ && Position(x, y) == tank_position_)
            return '%';

        return chars_grid_[x][y];
    }

    void convertGridToCharsGrid(const std::vector<std::vector<Cell>>& grid)
    {
        if (grid.empty() || grid[0].empty())
            return;

        size_t width = grid.size();
        size_t height = grid[0].size();

        chars_grid_.resize(width, std::vector<char>(height));

        for (size_t y = 0; y < height; ++y)
        {
            for (size_t x = 0; x < width; ++x)
            {
                chars_grid_[x][y] = grid[x][y].toChar();
            }
        }
    }

    bool outOfBounds(size_t x, size_t y) const
    {
        if (chars_grid_.empty() || chars_grid_[0].empty())
            return true;

        return x >= chars_grid_.size() || y >= chars_grid_[0].size();
    }

private:
    std::vector<std::vector<char>> chars_grid_;
    const Position tank_position_;
    bool for_tank_;
};
