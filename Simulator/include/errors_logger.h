#pragma once

#include <sstream>
#include <string>
#include <variant>
#include <vector>

namespace simulator
{
class ErrorsLogger
{
public:
    ErrorsLogger() = default;
    ~ErrorsLogger();

    ErrorsLogger(const ErrorsLogger&) = delete;
    ErrorsLogger& operator=(const ErrorsLogger&) = delete;
    ErrorsLogger(ErrorsLogger&&) = delete;
    ErrorsLogger& operator=(ErrorsLogger&&) = delete;

    // Log a general error
    template <typename... Args>
    void logGeneral(Args&&... args)
    {
        std::stringstream ss;
        (ss << ... << args);
        log_entries_.emplace_back(GeneralError{ss.str()});
    }

    // Log a file-specific error
    template <typename... Args>
    void logFile(const std::string& filename, Args&&... args)
    {
        std::stringstream ss;
        (ss << ... << args);

        if (!log_entries_.empty())
        {
            auto* last = std::get_if<FileErrors>(&log_entries_.back());
            if (last && last->filename == filename)
            {
                last->messages.push_back(ss.str());
                return;
            }
        }

        log_entries_.emplace_back(FileErrors{filename, {ss.str()}});
    }

private:
    void save_to_file(const std::string& filename) const;

    struct GeneralError
    {
        std::string message;
    };

    struct FileErrors
    {
        std::string filename;
        std::vector<std::string> messages;
    };

    std::vector<std::variant<GeneralError, FileErrors>> log_entries_;
};

} // namespace simulator