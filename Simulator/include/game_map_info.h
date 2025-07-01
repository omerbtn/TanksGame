#pragma once

#include <memory>

#include "SatelliteView.h"


struct GameMapInfo
{
    std::unique_ptr<SatelliteView> satellite_view;
    size_t height;
    size_t width;
    size_t max_steps;
    size_t num_shells;
    bool is_valid;

    GameMapInfo(std::unique_ptr<SatelliteView> satellite_view, size_t height, size_t width,
                size_t max_steps, size_t num_shells)
        : satellite_view(std::move(satellite_view)), height(height), width(width),
          max_steps(max_steps), num_shells(num_shells), is_valid(true) {}

    GameMapInfo() : is_valid(false) {}
};