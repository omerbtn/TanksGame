#pragma once

#include <vector>

#include "cell.h"
#include "SatelliteView.h"

class BoardSatelliteView : public SatelliteView {
public:
    BoardSatelliteView(const std::vector<std::vector<Cell>>& grid, const Position tank_position) : grid_(grid), tank_position_{tank_position} {
    }

    ~BoardSatelliteView() = default;

    char getObjectAt(size_t x, size_t y) const override {
        if (Position(x, y) == tank_position_) {
            return '%';
        }

        if (x >= grid_.size() || y >= grid_[0].size()) return '&';
        const auto& cell = grid_[x][y];
        if (cell.has(ObjectType::Wall)) return '#';
        if (cell.has(ObjectType::Shell)) return '*';
        if (cell.has(ObjectType::Mine)) return '@';
        if (cell.has(ObjectType::Tank)) {
            const auto& tanks = cell.get_objects_by_type(ObjectType::Tank);
            if (!tanks.empty()) {
                auto tank = std::static_pointer_cast<Tank>(tanks.front());
                return '0' + static_cast<char>(tank->player_id());
            }
        }
        return ' ';
    }

private:
    const std::vector<std::vector<Cell>>& grid_;
    const Position tank_position_;
};
