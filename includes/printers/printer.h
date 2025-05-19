#pragma once

#include "board.h"

template <typename Derived>
class Printer {
public:
    explicit Printer(const Board& board) : board_(board) {
    }

    void print() const {
        static_cast<const Derived*>(this)->print_impl();
    }

    const std::vector<std::vector<Cell>>& grid() const {
        return board_.grid();
    }

    size_t width() const {
        return board_.get_width();
    }

    size_t height() const {
        return board_.get_height();
    }

protected:
    const Board& board_;
};
