#pragma once
#include <cstdint>

#if defined(_WIN32)
    #define FRAMEWORK_PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
    #define FRAMEWORK_PLUGIN_EXPORT extern "C" __attribute__((visibility("default")))
#endif

namespace framework::runtime {
class IModule;
}

inline constexpr uint32_t FRAMEWORK_PLUGIN_API_VERSION = 1;
inline constexpr uint32_t FRAMEWORK_PLUGIN_ABI_VERSION = 1;

struct PluginVersion {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
};

struct PluginDescriptor {
    const char* id;
    const char* name;
    PluginVersion version;
    uint32_t apiVersion;
    uint32_t abiVersion;
    const char* const* dependencies;
    uint32_t dependencyCount;
};

#define FRAMEWORK_DECLARE_PLUGIN(ModuleClass, DescriptorInstance) \
    FRAMEWORK_PLUGIN_EXPORT const PluginDescriptor* get_plugin_descriptor() noexcept { \
        return &DescriptorInstance; \
    } \
    FRAMEWORK_PLUGIN_EXPORT framework::runtime::IModule* create_plugin_module() noexcept { \
        return new ModuleClass(); \
    } \
    FRAMEWORK_PLUGIN_EXPORT void destroy_plugin_module(framework::runtime::IModule* module) noexcept { \
        delete module; \
    }