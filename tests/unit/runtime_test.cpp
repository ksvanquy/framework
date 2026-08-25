#include "runtime/module_manager.h"
#include "runtime/runtime.h"
#include "example_module/example_module.h"
#include "services/default_services.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using framework::core::ErrorCode;
using framework::runtime::IModule;
using framework::runtime::ModuleInfo;
using framework::runtime::ModuleManager;
using framework::runtime::ModuleState;

class TestLogger final : public framework::services::ILogger {
public:
    void log(framework::services::LogLevel, std::string_view, std::string_view) override {}
};

class TestModule final : public IModule {
public:
    TestModule(std::string id, std::vector<std::string> dependencies = {},
               std::vector<std::string>* events = nullptr)
        : info_{std::move(id), "Test module", "1.0", std::move(dependencies)},
          events_(events) {}

    const ModuleInfo& info() const override { return info_; }
    ModuleState state() const override { return state_; }

    framework::core::Result<void> initialize() override {
        record("initialize");
        state_ = ModuleState::Initialized;
        return {};
    }

    framework::core::Result<void> start() override {
        record("start");
        if (failStart_) {
            return framework::core::Error(ErrorCode::InternalError, "start failed");
        }
        state_ = ModuleState::Started;
        return {};
    }

    framework::core::Result<void> stop() override {
        record("stop");
        state_ = ModuleState::Stopped;
        return {};
    }

    void setState(ModuleState state) { state_ = state; }
    void failStart() { failStart_ = true; }

private:
    void record(const char* action) {
        if (events_ != nullptr) {
            events_->push_back(std::string(action) + ":" + info_.id);
        }
    }

    ModuleInfo info_;
    ModuleState state_ = ModuleState::Discovered;
    std::vector<std::string>* events_;
    bool failStart_ = false;
};

std::unique_ptr<TestModule> module(std::string id,
                                   std::vector<std::string> dependencies = {},
                                   std::vector<std::string>* events = nullptr) {
    return std::make_unique<TestModule>(std::move(id), std::move(dependencies), events);
}

TEST(RuntimeTest, ModuleWithoutDependenciesCanInitializeAndStart) {
    TestLogger logger;
    ModuleManager manager(logger);
    auto standalone = module("standalone");
    auto* standalonePointer = standalone.get();
    ASSERT_TRUE(manager.registerModule(std::move(standalone)));

    EXPECT_TRUE(manager.initializeAll());
    EXPECT_EQ(standalonePointer->state(), ModuleState::Initialized);
    EXPECT_TRUE(manager.startAll());
    EXPECT_EQ(standalonePointer->state(), ModuleState::Started);
}

TEST(RuntimeTest, DependenciesInitializeAndStartBeforeDependent) {
    TestLogger logger;
    ModuleManager manager(logger);
    std::vector<std::string> events;
    ASSERT_TRUE(manager.registerModule(module("dependent", {"dependency"}, &events)));
    ASSERT_TRUE(manager.registerModule(module("dependency", {}, &events)));

    ASSERT_TRUE(manager.initializeAll());
    ASSERT_TRUE(manager.startAll());

    EXPECT_EQ(events, (std::vector<std::string>{
        "initialize:dependency", "initialize:dependent",
        "start:dependency", "start:dependent"}));
}

TEST(RuntimeTest, CircularDependencyIsRejected) {
    TestLogger logger;
    ModuleManager manager(logger);
    ASSERT_TRUE(manager.registerModule(module("a", {"b"})));
    ASSERT_TRUE(manager.registerModule(module("b", {"a"})));

    const auto result = manager.initializeAll();

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), ErrorCode::StateError);
    EXPECT_EQ(manager.getModuleState("a"), ModuleState::Discovered);
    EXPECT_EQ(manager.getModuleState("b"), ModuleState::Discovered);
}

TEST(RuntimeTest, MissingDependencyIsRejected) {
    TestLogger logger;
    ModuleManager manager(logger);
    ASSERT_TRUE(manager.registerModule(module("dependent", {"missing"})));

    const auto result = manager.initializeAll();

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), ErrorCode::NotFound);
}

TEST(RuntimeTest, FailedStartRollsBackModulesAlreadyStarted) {
    TestLogger logger;
    ModuleManager manager(logger);
    std::vector<std::string> events;
    auto dependency = module("dependency", {}, &events);
    auto* dependencyPointer = dependency.get();
    auto dependent = module("dependent", {"dependency"}, &events);
    dependent->failStart();
    auto* dependentPointer = dependent.get();
    ASSERT_TRUE(manager.registerModule(std::move(dependent)));
    ASSERT_TRUE(manager.registerModule(std::move(dependency)));
    ASSERT_TRUE(manager.initializeAll());

    const auto result = manager.startAll();

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), ErrorCode::InternalError);
    EXPECT_EQ(dependencyPointer->state(), ModuleState::Stopped);
    EXPECT_EQ(dependentPointer->state(), ModuleState::Initialized);
    EXPECT_EQ(events, (std::vector<std::string>{
        "initialize:dependency", "initialize:dependent",
        "start:dependency", "start:dependent", "stop:dependency"}));
}

TEST(RuntimeTest, StopUsesReverseDependencyOrder) {
    TestLogger logger;
    ModuleManager manager(logger);
    std::vector<std::string> events;
    ASSERT_TRUE(manager.registerModule(module("dependent", {"dependency"}, &events)));
    ASSERT_TRUE(manager.registerModule(module("dependency", {}, &events)));
    ASSERT_TRUE(manager.initializeAll());
    ASSERT_TRUE(manager.startAll());
    events.clear();

    ASSERT_TRUE(manager.stopAll());

    EXPECT_EQ(events, (std::vector<std::string>{"stop:dependent", "stop:dependency"}));
}

TEST(RuntimeTest, InvalidLifecycleStateIsRejected) {
    TestLogger logger;
    ModuleManager manager(logger);
    auto invalid = module("invalid");
    invalid->setState(ModuleState::Started);

    const auto registerResult = manager.registerModule(std::move(invalid));

    ASSERT_FALSE(registerResult);
    EXPECT_EQ(registerResult.error().code(), ErrorCode::StateError);
}

TEST(RuntimeTest, LifecycleTransitionContractAcceptsOnlyValidStates) {
    EXPECT_TRUE(framework::runtime::canInitialize(ModuleState::Discovered));
    EXPECT_TRUE(framework::runtime::canInitialize(ModuleState::Loaded));
    EXPECT_TRUE(framework::runtime::canInitialize(ModuleState::Stopped));
    EXPECT_FALSE(framework::runtime::canInitialize(ModuleState::Initialized));
    EXPECT_TRUE(framework::runtime::canStart(ModuleState::Initialized));
    EXPECT_FALSE(framework::runtime::canStart(ModuleState::Running));
    EXPECT_TRUE(framework::runtime::canStop(ModuleState::Started));
    EXPECT_TRUE(framework::runtime::canStop(ModuleState::Running));
    EXPECT_FALSE(framework::runtime::canStop(ModuleState::Initialized));
}

TEST(RuntimeTest, RuntimeCanRestartAfterStop) {
    framework::runtime::Runtime runtime;
    ASSERT_TRUE(runtime.moduleManager().registerModule(
        std::make_unique<framework::modules::ExampleModule>(runtime.logger(), runtime.eventBus())));

    ASSERT_TRUE(runtime.initialize());
    ASSERT_TRUE(runtime.start());
    ASSERT_TRUE(runtime.stop());
    ASSERT_TRUE(runtime.initialize());
    ASSERT_TRUE(runtime.start());
    EXPECT_TRUE(runtime.stop());
}

TEST(RuntimeTest, RuntimeContextProvidesNonOwningRuntimeServices) {
    framework::runtime::Runtime runtime;

    EXPECT_EQ(&runtime.context().logger, &runtime.logger());
    EXPECT_EQ(&runtime.context().eventBus, &runtime.eventBus());
    EXPECT_TRUE(&runtime.context().config != nullptr);
    EXPECT_TRUE(&runtime.context().commandBus != nullptr);
    EXPECT_TRUE(&runtime.context().scheduler != nullptr);
    EXPECT_TRUE(&runtime.context().storage != nullptr);
    EXPECT_TRUE(&runtime.context().diagnostics != nullptr);
}

TEST(RuntimeTest, DuplicateModuleIdIsRejected) {
    TestLogger logger;
    ModuleManager manager(logger);
    ASSERT_TRUE(manager.registerModule(module("duplicate")));

    const auto result = manager.registerModule(module("duplicate"));

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), ErrorCode::AlreadyExists);
}

TEST(RuntimeTest, ExampleModuleFollowsCompleteLifecycle) {
    TestLogger logger;
    framework::services::InMemoryEventBus eventBus;
    ModuleManager manager(logger);
    auto example = std::make_unique<framework::modules::ExampleModule>(logger, eventBus);
    auto* examplePointer = example.get();
    EXPECT_EQ(examplePointer->state(), ModuleState::Discovered);
    ASSERT_TRUE(manager.registerModule(std::move(example)));

    ASSERT_TRUE(manager.initializeAll());
    EXPECT_EQ(examplePointer->state(), ModuleState::Initialized);
    ASSERT_TRUE(manager.startAll());
    EXPECT_EQ(examplePointer->state(), ModuleState::Running);
    ASSERT_TRUE(manager.stopAll());
    EXPECT_EQ(examplePointer->state(), ModuleState::Stopped);
}

TEST(RuntimeTest, ExampleModuleUsesLoggerAndEventBusServices) {
    std::vector<std::string> logMessages;
    class CapturingLogger final : public framework::services::ILogger {
    public:
        explicit CapturingLogger(std::vector<std::string>& messages) : messages_(messages) {}
        void log(framework::services::LogLevel, std::string_view category,
                 std::string_view message) override {
            messages_.push_back(std::string(category) + ":" + std::string(message));
        }

    private:
        std::vector<std::string>& messages_;
    } logger(logMessages);
    framework::services::InMemoryEventBus eventBus;
    std::vector<std::string> events;
    auto started = eventBus.subscribe("example.started", [&](const void*) {
        events.push_back("started");
    });
    auto stopped = eventBus.subscribe("example.stopped", [&](const void*) {
        events.push_back("stopped");
    });
    ASSERT_NE(started, nullptr);
    ASSERT_NE(stopped, nullptr);

    framework::modules::ExampleModule example(logger, eventBus);
    ASSERT_TRUE(example.initialize());
    ASSERT_TRUE(example.start());
    ASSERT_TRUE(example.stop());

    EXPECT_EQ(events, (std::vector<std::string>{"started", "stopped"}));
    EXPECT_EQ(logMessages, (std::vector<std::string>{
        "Module:Example:Initializing", "Module:Example:Starting", "Module:Example:Stopping"}));
}

} // namespace