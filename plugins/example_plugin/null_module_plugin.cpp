#include "plugin_api.h"

namespace {

const PluginDescriptor descriptor{
    "null.module", "Null module plugin", {1, 0, 0},
    FRAMEWORK_PLUGIN_API_VERSION, FRAMEWORK_PLUGIN_ABI_VERSION, nullptr, 0};

} // namespace

FRAMEWORK_PLUGIN_EXPORT const PluginDescriptor* get_plugin_descriptor() noexcept {
    return &descriptor;
}

FRAMEWORK_PLUGIN_EXPORT framework::runtime::IModule* create_plugin_module() noexcept {
    return nullptr;
}

FRAMEWORK_PLUGIN_EXPORT void destroy_plugin_module(framework::runtime::IModule*) noexcept {}