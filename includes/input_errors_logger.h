#pragma once

#include <sstream>
#include <string>
#include <vector>

class InputErrorLogger
{
public:
    InputErrorLogger() = default;
    ~InputErrorLogger();

    InputErrorLogger(const InputErrorLogger&) = delete;
    InputErrorLogger& operator=(const InputErrorLogger&) = delete;
    InputErrorLogger(InputErrorLogger&&) = delete;
    InputErrorLogger& operator=(InputErrorLogger&&) = delete;

    template <typename... Args>
    void log(Args&&... args)
    {
        std::stringstream ss;
        (ss << ... << args);
        errors_.push_back(ss.str());
    }

private:
    void save_to_file(const std::string& filename) const;

    std::vector<std::string> errors_; // Accumulate error messages
};
