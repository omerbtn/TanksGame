#include "input_errors_logger.h"

#include <fstream>
#include <iostream>

#include "global_config.h"

InputErrorLogger::~InputErrorLogger()
{
    save_to_file(static_cast<std::string>(config::get<std::string_view>("input_error_file")));
}

void InputErrorLogger::save_to_file(const std::string& filename) const
{
    if (errors_.empty())
    {
        return;
    }

    std::ofstream out(filename);
    if (!out.is_open())
    {
        std::cerr << "Warning: Failed to create " << filename << std::endl;
        return;
    }

    for (const auto& error : errors_)
    {
        out << error << std::endl;
    }
}
