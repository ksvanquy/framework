#include "example_module/example_module.h"
#include "runtime/application.h"
#include "runtime/module_manager.h"
#include "runtime/runtime.h"

#include <memory>
#include <string_view>

class MiniApplication final : public framework::runtime::Application {
protected:
    framework::core::Result<void> onConfigureModules(framework::runtime::Runtime& runtime) override {
        startedSubscription_ = runtime.context().eventBus.subscribe(
            "example.started", [this](const void*) { onModuleEvent("Example module started"); });
        stoppedSubscription_ = runtime.context().eventBus.subscribe(
            "example.stopped", [this](const void*) { onModuleEvent("Example module stopped"); });
        if (startedSubscription_ == nullptr || stoppedSubscription_ == nullptr) {
            return framework::core::Error(
                framework::core::ErrorCode::InternalError, "Miniapp could not subscribe to module events");
        }

        return runtime.moduleManager().registerModule(
            std::make_unique<framework::modules::ExampleModule>(
                runtime.context().logger, runtime.context().eventBus));
    }

    int onRun() override {
        runtime_.context().logger.log(
            framework::services::LogLevel::Info, "Application:Miniapp", "Application is running");
        return 0;
    }

private:
    void onModuleEvent(std::string_view message) {
        runtime_.context().logger.log(
            framework::services::LogLevel::Info, "Application:Miniapp", message);
    }

    std::unique_ptr<framework::services::SubscriptionToken> startedSubscription_;
    std::unique_ptr<framework::services::SubscriptionToken> stoppedSubscription_;
};

int main(int argc, char* argv[]) {
    MiniApplication application;
    return application.exec(argc, argv);
}