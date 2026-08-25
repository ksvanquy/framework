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
    framework::runtime::ModuleInfo info_{"example.plugin", "Example plugin", "1.0.0", {}};
    framework::runtime::ModuleState state_ = framework::runtime::ModuleState::Discovered;
};

const PluginDescriptor descriptor{
    "example.plugin", "Example plugin", {1, 0, 0},
    FRAMEWORK_PLUGIN_API_VERSION, FRAMEWORK_PLUGIN_ABI_VERSION, nullptr, 0};

} // namespace

FRAMEWORK_DECLARE_PLUGIN(PluginModule, descriptor)