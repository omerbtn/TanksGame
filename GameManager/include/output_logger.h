#pragma once

#include <fstream>
#include <iostream>
#include <optional>

#include "ActionRequest.h"
#include "tank.h"

class OutputLogger
{
public:
    OutputLogger() = default;
    OutputLogger(const std::string& filename, const size_t total_tanks);

    OutputLogger(const OutputLogger&) = delete;
    OutputLogger& operator=(const OutputLogger&) = delete;

    OutputLogger(OutputLogger&&) = default;
    OutputLogger& operator=(OutputLogger&&) = default;

    bool is_valid() const;

    void logAction(size_t tank_no, std::optional<ActionRequest> action, bool valid, bool was_alive_at_start, bool died_this_round);
    void logResult(std::string&& result);

private:
    std::ofstream out_;
    size_t total_tanks_ = 0;
    bool valid_ = false;
};
