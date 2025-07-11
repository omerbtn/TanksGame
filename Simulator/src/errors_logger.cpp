#include "errors_logger.h"

#include <fstream>
#include <iostream>

#include "global_config.h"

using namespace UserCommon_322573304_322647603;


ErrorsLogger::~ErrorsLogger()
{
    save_to_file(std::string(config::get<std::string_view>("input_error_file")));
}

void ErrorsLogger::save_to_file(const std::string& filename) const
{
    if (log_entries_.empty())
    {
        return;
    }

    std::ofstream out(filename);
    if (!out.is_open())
    {
        std::cerr << "Warning: Failed to create " << filename << std::endl;
        return;
    }

    for (const auto& entry : log_entries_)
    {
        if (std::holds_alternative<GeneralError>(entry))
        {
            const auto& err = std::get<GeneralError>(entry);
            out << err.message << "\n\n";
        }
        else if (std::holds_alternative<FileErrors>(entry))
        {
            const auto& err = std::get<FileErrors>(entry);
            out << "Errors in " << err.filename << ":\n";
            for (const auto& msg : err.messages)
            {
                out << "  - " << msg << '\n';
            }
            out << '\n';
        }
    }
}
