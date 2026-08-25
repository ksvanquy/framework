#include "plugin_api.h"
#include "runtime/imodule.h"

namespace {

class PluginModule final : public framework::runtime::IModule {
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
    framework::runtime::ModuleInfo info_{"invalid.plugin", "Invalid plugin", "1.0.0", {}};
    framework::runtime::ModuleState state_ = framework::runtime::ModuleState::Discovered;
};

const PluginDescriptor descriptor{
    "invalid.plugin", "Invalid plugin", {1, 0, 0},
#if defined(INVALID_PLUGIN_API_VERSION)
    INVALID_PLUGIN_API_VERSION,
#else
    FRAMEWORK_PLUGIN_API_VERSION,
#endif
#if defined(INVALID_PLUGIN_ABI_VERSION)
    INVALID_PLUGIN_ABI_VERSION,
#else
    FRAMEWORK_PLUGIN_ABI_VERSION,
#endif
    nullptr, 0};

} // namespace

FRAMEWORK_DECLARE_PLUGIN(PluginModule, descriptor)