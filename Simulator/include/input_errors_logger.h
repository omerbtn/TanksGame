#pragma once

#include <sstream>
#include <string>
#include <vector>

class InputErrorsLogger
{
public:
    InputErrorsLogger() = default;
    ~InputErrorsLogger();

    InputErrorsLogger(const InputErrorsLogger&) = delete;
    InputErrorsLogger& operator=(const InputErrorsLogger&) = delete;
    InputErrorsLogger(InputErrorsLogger&&) = delete;
    InputErrorsLogger& operator=(InputErrorsLogger&&) = delete;

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
