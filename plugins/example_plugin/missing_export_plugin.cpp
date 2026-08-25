#include "plugin_api.h"
#include "runtime/imodule.h"

namespace {

class MissingExportModule final : public framework::runtime::IModule {
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
    framework::runtime::ModuleInfo info_{"missing.export", "Missing export plugin", "1.0.0", {}};
    framework::runtime::ModuleState state_ = framework::runtime::ModuleState::Discovered;
};

const PluginDescriptor descriptor{
    "missing.export", "Missing export plugin", {1, 0, 0},
    FRAMEWORK_PLUGIN_API_VERSION, FRAMEWORK_PLUGIN_ABI_VERSION, nullptr, 0};

} // namespace

#if !defined(MISSING_PLUGIN_DESCRIPTOR)
FRAMEWORK_PLUGIN_EXPORT const PluginDescriptor* get_plugin_descriptor() noexcept {
    return &descriptor;
}
#endif

#if !defined(MISSING_PLUGIN_CREATE)
FRAMEWORK_PLUGIN_EXPORT framework::runtime::IModule* create_plugin_module() noexcept {
    return new MissingExportModule();
}
#endif

#if !defined(MISSING_PLUGIN_DESTROY)
FRAMEWORK_PLUGIN_EXPORT void destroy_plugin_module(framework::runtime::IModule* module) noexcept {
    delete module;
}
#endif