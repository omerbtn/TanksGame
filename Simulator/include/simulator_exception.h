#pragma once

#include <stdexcept>
#include <string>


namespace simulator
{
class SimulatorException : public std::runtime_error
{
public:
    explicit SimulatorException(const std::string& message)
        : std::runtime_error(message) {}
};

} // namespace simulator