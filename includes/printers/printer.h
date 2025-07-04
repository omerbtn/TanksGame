#pragma once

#include "board.h"
#include "cell.h"

template <typename Derived>
class Printer
{
public:
    explicit Printer(const std::vector<std::vector<Cell>>& grid) : grid_(grid) {}

    void print() const
    {
        static_cast<const Derived*>(this)->printImpl();
    }

    const std::vector<std::vector<Cell>>& grid() const
    {
        return grid_;
    }

    size_t width() const
    {
        return grid_[0].size();
    }

    size_t height() const
    {
        return grid_.size();
    }

protected:
    const std::vector<std::vector<Cell>>& grid_;
};
