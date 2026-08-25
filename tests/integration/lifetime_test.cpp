#include "runtime/module_manager.h"
#include "runtime/plugin_loader.h"
#include "runtime/runtime.h"
#include "services/default_services.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <string_view>

namespace {

class SilentLogger final : public framework::services::ILogger {
public:
    void log(framework::services::LogLevel, std::string_view, std::string_view) override {}
};

class EventSubscriptionModule final : public framework::runtime::IModule {
public:
    EventSubscriptionModule(framework::services::IEventBus& eventBus, int& callbackCount)
        : eventBus_(eventBus), callbackCount_(callbackCount) {}

    const framework::runtime::ModuleInfo& info() const override { return info_; }
    framework::runtime::ModuleState state() const override { return state_; }

    framework::core::Result<void> initialize() override {
        subscription_ = eventBus_.subscribe("lifetime.test", [this](const void*) {
            ++callbackCount_;
        });
        if (subscription_ == nullptr) {
            return framework::core::Error(
                framework::core::ErrorCode::InternalError, "Unable to subscribe to event bus");
        }
        state_ = framework::runtime::ModuleState::Initialized;
        return {};
    }

    framework::core::Result<void> start() override {
        state_ = framework::runtime::ModuleState::Running;
        return {};
    }

    framework::core::Result<void> stop() override {
        subscription_.reset();
        state_ = framework::runtime::ModuleState::Stopped;
        return {};
    }

private:
    framework::runtime::ModuleInfo info_{"subscription", "Subscription module", "1.0.0", {}};
    framework::services::IEventBus& eventBus_;
    int& callbackCount_;
    std::unique_ptr<framework::services::SubscriptionToken> subscription_;
    framework::runtime::ModuleState state_ = framework::runtime::ModuleState::Discovered;
};

TEST(LifetimeIntegrationTest, EventSubscriptionIsRemovedWhenModuleIsDestroyed) {
    SilentLogger logger;
    framework::services::InMemoryEventBus eventBus;
    int callbackCount = 0;

    {
        framework::runtime::ModuleManager manager(logger);
        ASSERT_TRUE(manager.registerModule(
            std::make_unique<EventSubscriptionModule>(eventBus, callbackCount)));
        ASSERT_TRUE(manager.initializeAll());
        eventBus.publish("lifetime.test", nullptr);
        EXPECT_EQ(callbackCount, 1);
    }

    eventBus.publish("lifetime.test", nullptr);
    EXPECT_EQ(callbackCount, 1);
}

TEST(LifetimeIntegrationTest, RuntimeStopUnloadsRegisteredPlugin) {
    framework::runtime::PluginLoader loader;
    auto plugin = loader.load(FRAMEWORK_EXAMPLE_PLUGIN_PATH);
    ASSERT_TRUE(plugin);

    framework::runtime::Runtime runtime;
    ASSERT_TRUE(runtime.moduleManager().registerPlugin(std::move(plugin.value())));
    ASSERT_TRUE(runtime.initialize());
    ASSERT_TRUE(runtime.start());
    ASSERT_EQ(runtime.moduleManager().getModuleState("example.plugin"),
              framework::runtime::ModuleState::Running);

    ASSERT_TRUE(runtime.stop());
    EXPECT_EQ(runtime.moduleManager().getModuleState("example.plugin"),
              framework::runtime::ModuleState::Unloaded);
}

} // namespace