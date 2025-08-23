#include "loader/shared_library_loader.h"

#include "arguments_parser.h"

namespace simulator
{
namespace loader
{

std::shared_ptr<void> SharedLibraryLoader::load(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = shared_objects_.find(path);
    if (it != shared_objects_.end()) {
        return it->second;
    }

    void* handle = dlopen(path.c_str(), RTLD_LAZY | RTLD_GLOBAL);
    if (!handle) {
        return nullptr;
    }

    auto deleter = [](void* h) {
        if (h) {
            if (dlclose(h) != 0) {
                std::cerr << "Failed to close shared library: " << dlerror() << std::endl;
            }
        }
    };

    std::shared_ptr<void> shared_handle(handle, deleter);
    shared_objects_[path] = shared_handle;
    return shared_handle;
}

} // namespace loader
} // namespace simulator