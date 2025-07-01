#pragma once

#include <stdexcept>
#include <string>


class SimulatorException : public std::runtime_error
{
public:
    explicit SimulatorException(const std::string& message)
        : std::runtime_error(message) {}
};
