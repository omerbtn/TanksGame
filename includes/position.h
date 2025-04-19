#pragma once

class Position {
public:
    Position() = default;
    Position(int x, int y) : x(x), y(y) {}
    int x = 0;
    int y = 0;
};