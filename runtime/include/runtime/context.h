#pragma once

#include "services/icommand_bus.h"
#include "services/iconfig.h"
#include "services/idiagnostics.h"
#include "services/ievent_bus.h"
#include "services/ilogger.h"
#include "services/ischeduler.h"
#include "services/istorage.h"

namespace framework::runtime {

struct RuntimeContext {
    services::ILogger& logger;
    services::IEventBus& eventBus;
    services::IConfig& config;
    services::ICommandBus& commandBus;
    services::IScheduler& scheduler;
    services::IStorage& storage;
    services::IDiagnostics& diagnostics;
};

} // namespace framework::runtime