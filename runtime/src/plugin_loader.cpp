#include "runtime/plugin_loader.h"

#include <string>
#include <unordered_set>
#include <utility>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace framework::runtime {
namespace {

core::Result<LoadedPlugin> error(core::ErrorCode code, std::string message) {
    return core::Error(code, std::move(message));
}

using DescriptorFunction = const PluginDescriptor* (*)();
using CreateFunction = IModule* (*)();
using DestroyFunction = void (*)(IModule*);

void closeLibrary(void* handle) {
#if defined(_WIN32)
    FreeLibrary(static_cast<HMODULE>(handle));
#else
    dlclose(handle);
#endif
}

void* findSymbol(void* handle, const char* name) {
#if defined(_WIN32)
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle), name));
#else
    return dlsym(handle, name);
#endif
}

} // namespace

LoadedPlugin::LoadedPlugin(const PluginDescriptor& descriptor, IModule* module,
                           DestroyModule destroyModule, std::shared_ptr<void> library)
    : descriptor_(&descriptor), module_(module), destroyModule_(destroyModule),
      library_(std::move(library)) {}

LoadedPlugin::~LoadedPlugin() { reset(); }

LoadedPlugin::LoadedPlugin(LoadedPlugin&& other) noexcept
    : descriptor_(other.descriptor_), module_(other.module_),
      destroyModule_(other.destroyModule_), library_(std::move(other.library_)) {
    other.descriptor_ = nullptr;
    other.module_ = nullptr;
    other.destroyModule_ = nullptr;
}

LoadedPlugin& LoadedPlugin::operator=(LoadedPlugin&& other) noexcept {
    if (this != &other) {
        reset();
        descriptor_ = other.descriptor_;
        module_ = other.module_;
        destroyModule_ = other.destroyModule_;
        library_ = std::move(other.library_);
        other.descriptor_ = nullptr;
        other.module_ = nullptr;
        other.destroyModule_ = nullptr;
    }
    return *this;
}

const PluginDescriptor& LoadedPlugin::descriptor() const { return *descriptor_; }
IModule& LoadedPlugin::module() const { return *module_; }

void LoadedPlugin::reset() noexcept {
    if (module_ != nullptr && destroyModule_ != nullptr) {
        destroyModule_(module_);
    }
    module_ = nullptr;
    destroyModule_ = nullptr;
    descriptor_ = nullptr;
    library_.reset();
}

core::Result<LoadedPlugin> PluginLoader::load(const std::filesystem::path& path) const {
#if defined(_WIN32)
    void* handle = static_cast<void*>(LoadLibraryW(path.wstring().c_str()));
#else
    void* handle = dlopen(path.string().c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
    if (handle == nullptr) {
        return error(core::ErrorCode::PluginLoadFailed, "Unable to load plugin: " + path.string());
    }

    auto library = std::shared_ptr<void>(handle, closeLibrary);
    const auto descriptorFunction = reinterpret_cast<DescriptorFunction>(findSymbol(handle, "get_plugin_descriptor"));
    const auto createFunction = reinterpret_cast<CreateFunction>(findSymbol(handle, "create_plugin_module"));
    const auto destroyFunction = reinterpret_cast<DestroyFunction>(findSymbol(handle, "destroy_plugin_module"));
    if (descriptorFunction == nullptr) {
        return error(core::ErrorCode::PluginLoadFailed,
                     "Plugin export is missing: get_plugin_descriptor: " + path.string());
    }
    if (createFunction == nullptr) {
        return error(core::ErrorCode::PluginLoadFailed,
                     "Plugin export is missing: create_plugin_module: " + path.string());
    }
    if (destroyFunction == nullptr) {
        return error(core::ErrorCode::PluginLoadFailed,
                     "Plugin export is missing: destroy_plugin_module: " + path.string());
    }

    const PluginDescriptor* descriptor = descriptorFunction();
    if (descriptor == nullptr || descriptor->id == nullptr || descriptor->name == nullptr ||
        descriptor->id[0] == '\0' || descriptor->name[0] == '\0') {
        return error(core::ErrorCode::InvalidArgument, "Plugin descriptor is invalid: " + path.string());
    }
    if (descriptor->apiVersion != CurrentApiVersion) {
        return error(core::ErrorCode::PluginLoadFailed, "Plugin API version is incompatible: " + path.string());
    }
    if (descriptor->abiVersion != CurrentAbiVersion) {
        return error(core::ErrorCode::PluginLoadFailed, "Plugin ABI version is incompatible: " + path.string());
    }
    if (descriptor->dependencyCount > 0 && descriptor->dependencies == nullptr) {
        return error(core::ErrorCode::InvalidArgument, "Plugin dependencies are invalid: " + path.string());
    }
    std::unordered_set<std::string> dependencies;
    for (uint32_t index = 0; index < descriptor->dependencyCount; ++index) {
        const char* dependency = descriptor->dependencies[index];
        if (dependency == nullptr || dependency[0] == '\0' || !dependencies.emplace(dependency).second) {
            return error(core::ErrorCode::InvalidArgument, "Plugin dependencies are invalid: " + path.string());
        }
    }

    IModule* module = createFunction();
    if (module == nullptr) {
        return error(core::ErrorCode::PluginLoadFailed, "Plugin module creation failed: " + path.string());
    }
    if (module->info().id != descriptor->id) {
        destroyFunction(module);
        return error(core::ErrorCode::InvalidArgument, "Plugin module ID does not match descriptor: " + path.string());
    }
    return LoadedPlugin(*descriptor, module, destroyFunction, std::move(library));
}

} // namespace framework::runtime