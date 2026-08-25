#pragma once

#include "runtime/imodule.h"
#include "services/ievent_bus.h"
#include "services/ilogger.h"

namespace framework::modules {

class ExampleModule final : public runtime::IModule {
public:
    ExampleModule(services::ILogger& logger, services::IEventBus& eventBus);

    const runtime::ModuleInfo& info() const override;
    runtime::ModuleState state() const override;
    core::Result<void> initialize() override;
    core::Result<void> start() override;
    core::Result<void> stop() override;

private:
    runtime::ModuleInfo info_;
    services::ILogger& logger_;
    services::IEventBus& eventBus_;
    runtime::ModuleState state_ = runtime::ModuleState::Discovered;
};

} // namespace framework::modules