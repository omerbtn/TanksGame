#pragma once

#include "shared_library_loader.h"

#include <string>
#include <filesystem>

#include "arguments_parser.h"
#include "global_config.h"
#include "errors_logger.h"
#include "simulator_exception.h"
#include "registrations/registrar_adapter.h"

namespace simulator
{
namespace loader
{

class SharedObjectLoader {
private:
    SharedLibraryLoader shared_library_loader_;
    ErrorsLogger& errors_logger_;

public:
    explicit SharedObjectLoader(ErrorsLogger& errors_logger): errors_logger_(errors_logger) {}
    SharedObjectLoader(const SharedObjectLoader&) = delete;
    SharedObjectLoader& operator=(const SharedObjectLoader&) = delete;
    SharedObjectLoader(SharedObjectLoader&&) = delete;
    SharedObjectLoader& operator=(SharedObjectLoader&&) = delete;

    template <typename Registrar>
    bool loadSharedObject(const std::string& path);
    
    template <typename Registrar>
    size_t loadSharedObjectsFromFolder(const std::string& folder_path);
};

namespace fs = std::filesystem;
using namespace UserCommon_322573304_322647603;

template <typename Registrar>
inline size_t SharedObjectLoader::loadSharedObjectsFromFolder(const std::string& folder_path)
{
    // Check if the folder exists and is a directory
    if (!fs::exists(folder_path) || !fs::is_directory(folder_path))
    {
        std::ostringstream oss;
        ArgumentsParser::printUsage(oss, "Folder does not exist or is not a directory: " + folder_path);
        throw SimulatorException(oss.str());
    }

    // Iterate through all files in the folder and load valid shared objects
    size_t loaded_count = 0;
    for (const auto& entry : fs::directory_iterator(folder_path))
    {
        if (!entry.is_regular_file() || entry.path().extension() != config::get<std::string_view>("shared_library_extension"))
            continue;

        std::string path = entry.path().string();
        if (loadSharedObject<Registrar>(path))
        {
            ++loaded_count;
        }
    }

    return loaded_count;
}

template <typename Registrar>
inline bool SharedObjectLoader::loadSharedObject(const std::string& path)
{
    std::string name = fs::path(path).stem().string();
    registrations::RegistrarAdapter<Registrar>::createEntry(name);

    if (auto shared_obj = shared_library_loader_.load(path); !shared_obj)
    {
        errors_logger_.logGeneral("dlopen failed for ", path, ": ", dlerror());
        registrations::RegistrarAdapter<Registrar>::removeLast();
        return false;
    }

    try
    {
        registrations::RegistrarAdapter<Registrar>::validateLast();
    }
    catch (const typename registrations::RegistrarAdapter<Registrar>::BadRegistrationException& e)
    {
        errors_logger_.logGeneral(registrations::RegistrarAdapter<Registrar>::getBadRegistrationDetails(e));
        registrations::RegistrarAdapter<Registrar>::removeLast();
        return false;
    }

    return true;
}

} // namespace loader
} // namespace simulator