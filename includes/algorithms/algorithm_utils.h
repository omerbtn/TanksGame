#pragma once

#include "types/direction.h"

Direction getDirectionTo(int sx, int sy, int tx, int ty)
{
    int dx = tx - sx;
    int dy = sy - ty;

    if (dx == 0 && dy > 0)
        return Direction::U;
    if (dx > 0 && dy > 0)
        return Direction::UR;
    if (dx > 0 && dy == 0)
        return Direction::R;
    if (dx > 0 && dy < 0)
        return Direction::DR;
    if (dx == 0 && dy < 0)
        return Direction::D;
    if (dx < 0 && dy < 0)
        return Direction::DL;
    if (dx < 0 && dy == 0)
        return Direction::L;
    if (dx < 0 && dy > 0)
        return Direction::UL;

    return Direction::U; // fallback
}