#include "runtime/runtime.h"

#include "runtime/module_manager.h"
#include "services/default_services.h"

#include <iostream>
#include <utility>

namespace framework::runtime {

class Runtime::Impl {
public:
    Impl()
        : logger(std::clog),
          moduleManager(logger) {}

    services::ConsoleLogger logger;
    services::InMemoryConfig config;
    services::InMemoryEventBus eventBus;
    services::InMemoryCommandBus commandBus;
    services::ThreadScheduler scheduler;
    services::InMemoryStorage storage;
    services::BasicDiagnostics diagnostics;
    RuntimeContext context{logger, eventBus, config, commandBus, scheduler, storage, diagnostics};
    ModuleManager moduleManager;
    bool initialized = false;
    bool started = false;
};

Runtime::Runtime() : impl_(std::make_unique<Impl>()) {}

Runtime::~Runtime() {
    stop();
}

core::Result<void> Runtime::initialize() {
    if (impl_->initialized) {
        return {};
    }
    auto result = impl_->moduleManager.initializeAll();
    if (!result) {
        impl_->logger.log(services::LogLevel::Error, "Runtime", "Runtime initialization failed");
        return result;
    }
    impl_->initialized = true;
    impl_->logger.log(services::LogLevel::Info, "Runtime", "Runtime initialized");
    return {};
}

core::Result<void> Runtime::start() {
    if (!impl_->initialized) {
        return core::Error(core::ErrorCode::StateError, "Runtime must be initialized before start");
    }
    if (impl_->started) {
        return {};
    }
    auto result = impl_->moduleManager.startAll();
    if (!result) {
        impl_->logger.log(services::LogLevel::Error, "Runtime", "Runtime start failed");
        return result;
    }
    impl_->started = true;
    impl_->logger.log(services::LogLevel::Info, "Runtime", "Runtime started");
    return {};
}

core::Result<void> Runtime::stop() {
    if (!impl_) {
        return {};
    }
    if (!impl_->initialized) {
        return impl_->moduleManager.unloadAllPlugins();
    }
    if (!impl_->started) {
        impl_->initialized = false;
        return impl_->moduleManager.unloadAllPlugins();
    }

    auto result = impl_->moduleManager.stopAll();
    auto unloadResult = impl_->moduleManager.unloadAllPlugins();
    impl_->started = false;
    impl_->initialized = false;
    if (!result) {
        impl_->logger.log(services::LogLevel::Error, "Runtime", "Runtime stop failed");
        return result;
    }
    if (!unloadResult) {
        impl_->logger.log(services::LogLevel::Error, "Runtime", "Plugin unload failed");
        return unloadResult;
    }
    impl_->logger.log(services::LogLevel::Info, "Runtime", "Runtime stopped");
    return {};
}

services::ILogger& Runtime::logger() const {
    return impl_->logger;
}

services::IEventBus& Runtime::eventBus() const {
    return impl_->eventBus;
}

RuntimeContext& Runtime::context() {
    return impl_->context;
}

ModuleManager& Runtime::moduleManager() {
    return impl_->moduleManager;
}

} // namespace framework::runtime
