#include "plugin_api.h"
#include "runtime/imodule.h"

namespace {

class MalformedDescriptorModule final : public framework::runtime::IModule {
public:
    const framework::runtime::ModuleInfo& info() const override { return info_; }
    framework::runtime::ModuleState state() const override { return state_; }

    framework::core::Result<void> initialize() override {
        state_ = framework::runtime::ModuleState::Initialized;
        return {};
    }

    framework::core::Result<void> start() override {
        state_ = framework::runtime::ModuleState::Running;
        return {};
    }

    framework::core::Result<void> stop() override {
        state_ = framework::runtime::ModuleState::Stopped;
        return {};
    }

private:
    framework::runtime::ModuleInfo info_{"malformed.descriptor", "Malformed descriptor", "1.0.0", {}};
    framework::runtime::ModuleState state_ = framework::runtime::ModuleState::Discovered;
};

#if defined(MALFORMED_DESCRIPTOR_NULL)
const PluginDescriptor* descriptor = nullptr;
#elif defined(MALFORMED_DESCRIPTOR_EMPTY_ID)
const PluginDescriptor descriptor{"", "Malformed descriptor", {1, 0, 0},
    FRAMEWORK_PLUGIN_API_VERSION, FRAMEWORK_PLUGIN_ABI_VERSION, nullptr, 0};
#elif defined(MALFORMED_DESCRIPTOR_EMPTY_NAME)
const PluginDescriptor descriptor{"malformed.descriptor", "", {1, 0, 0},
    FRAMEWORK_PLUGIN_API_VERSION, FRAMEWORK_PLUGIN_ABI_VERSION, nullptr, 0};
#elif defined(MALFORMED_DESCRIPTOR_NULL_DEPENDENCIES)
const PluginDescriptor descriptor{"malformed.descriptor", "Malformed descriptor", {1, 0, 0},
    FRAMEWORK_PLUGIN_API_VERSION, FRAMEWORK_PLUGIN_ABI_VERSION, nullptr, 1};
#else
const char* duplicateDependencies[] = {"dependency", "dependency"};
const PluginDescriptor descriptor{"malformed.descriptor", "Malformed descriptor", {1, 0, 0},
    FRAMEWORK_PLUGIN_API_VERSION, FRAMEWORK_PLUGIN_ABI_VERSION, duplicateDependencies, 2};
#endif

} // namespace

FRAMEWORK_PLUGIN_EXPORT const PluginDescriptor* get_plugin_descriptor() noexcept {
#if defined(MALFORMED_DESCRIPTOR_NULL)
    return descriptor;
#else
    return &descriptor;
#endif
}

FRAMEWORK_PLUGIN_EXPORT framework::runtime::IModule* create_plugin_module() noexcept {
    return new MalformedDescriptorModule();
}

FRAMEWORK_PLUGIN_EXPORT void destroy_plugin_module(framework::runtime::IModule* module) noexcept {
    delete module;
}