#pragma once

#include <dlfcn.h>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <iostream>

#include "simulator_exception.h"

namespace simulator
{
namespace loader
{

class SharedLibraryLoader {
public:
    SharedLibraryLoader() = default;
    ~SharedLibraryLoader() = default;

    SharedLibraryLoader(const SharedLibraryLoader&) = delete;
    SharedLibraryLoader& operator=(const SharedLibraryLoader&) = delete;
    SharedLibraryLoader(SharedLibraryLoader&&) = delete;
    SharedLibraryLoader& operator=(SharedLibraryLoader&&) = delete;

    std::shared_ptr<void> load(const std::string& path);
    void unload(const std::string& path);

private:
    std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<void>> shared_objects_;
};

} // namespace loader
} // namespace simulator