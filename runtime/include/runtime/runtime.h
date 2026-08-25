#pragma once
#include <memory>
#include "context.h"
#include "core/result.h"
#include "services/ilogger.h"
#include "services/ievent_bus.h"

namespace framework::runtime {

class ModuleManager;

class Runtime {
public:
    Runtime();
    ~Runtime();

    // Lifecycle: initialize -> start -> stop. Each operation is idempotent where applicable.
    core::Result<void> initialize();
    core::Result<void> start();
    core::Result<void> stop();

    services::ILogger& logger() const;
    services::IEventBus& eventBus() const;
    RuntimeContext& context();
    ModuleManager& moduleManager();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace framework::runtime