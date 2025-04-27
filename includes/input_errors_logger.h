#pragma once

#include <vector>
#include <string>
#include <sstream>

class InputErrorLogger {
public:
    InputErrorLogger() = default;
    ~InputErrorLogger();

    template <typename... Args>
    void log(Args&&... args) {
        std::stringstream ss;
        (ss << ... << args);
        errors_.push_back(ss.str());
    }

private:
    void save_to_file(const std::string& filename) const;

    std::vector<std::string> errors_;  // Accumulate error messages
};
