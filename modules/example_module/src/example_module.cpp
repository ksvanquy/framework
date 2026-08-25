#include "example_module/example_module.h"

namespace framework::modules {

ExampleModule::ExampleModule(services::ILogger& logger, services::IEventBus& eventBus)
    : info_{"example", "Example module", "1.0.0", {}}, logger_(logger), eventBus_(eventBus) {}

const runtime::ModuleInfo& ExampleModule::info() const { return info_; }
runtime::ModuleState ExampleModule::state() const { return state_; }

core::Result<void> ExampleModule::initialize() {
    if (state_ != runtime::ModuleState::Discovered) {
        return core::Error(core::ErrorCode::StateError, "Example module cannot be initialized");
    }
    logger_.log(services::LogLevel::Info, "Module:Example", "Initializing");
    state_ = runtime::ModuleState::Initialized;
    return {};
}

core::Result<void> ExampleModule::start() {
    if (state_ != runtime::ModuleState::Initialized) {
        return core::Error(core::ErrorCode::StateError, "Example module cannot be started");
    }
    logger_.log(services::LogLevel::Info, "Module:Example", "Starting");
    state_ = runtime::ModuleState::Running;
    eventBus_.publish("example.started", nullptr);
    return {};
}

core::Result<void> ExampleModule::stop() {
    if (state_ != runtime::ModuleState::Running) {
        return core::Error(core::ErrorCode::StateError, "Example module cannot be stopped");
    }
    logger_.log(services::LogLevel::Info, "Module:Example", "Stopping");
    state_ = runtime::ModuleState::Stopped;
    eventBus_.publish("example.stopped", nullptr);
    return {};
}

} // namespace framework::modules