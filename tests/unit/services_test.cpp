#include "services/default_services.h"

#include <gtest/gtest.h>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

using framework::core::ErrorCode;
using namespace framework::services;

TEST(ConfigTest, StoresAndOverwritesValues) {
    InMemoryConfig config;

    ASSERT_TRUE(config.setString("theme", "light"));
    ASSERT_TRUE(config.setString("theme", "dark"));
    const auto result = config.getString("theme");

    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), "dark");
}

TEST(ConfigTest, RejectsEmptyKey) {
    InMemoryConfig config;

    const auto result = config.setString("", "value");

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), ErrorCode::InvalidArgument);
}

TEST(ConfigTest, ReportsMissingKey) {
    InMemoryConfig config;

    const auto result = config.getString("missing");

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), ErrorCode::NotFound);
}

TEST(StorageTest, StoresAndOverwritesValues) {
    InMemoryStorage storage;

    ASSERT_TRUE(storage.set("user", "alice"));
    ASSERT_TRUE(storage.set("user", "bob"));
    const auto result = storage.get("user");

    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), "bob");
}

TEST(StorageTest, RejectsEmptyKey) {
    InMemoryStorage storage;

    const auto result = storage.set("", "value");

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), ErrorCode::InvalidArgument);
}

TEST(StorageTest, ReportsMissingKey) {
    InMemoryStorage storage;

    const auto result = storage.get("missing");

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), ErrorCode::NotFound);
}

TEST(EventBusTest, DeliversPublishedDataToSubscribers) {
    InMemoryEventBus eventBus;
    int payload = 42;
    const void* received = nullptr;
    auto token = eventBus.subscribe("data", [&](const void* data) { received = data; });
    ASSERT_NE(token, nullptr);

    eventBus.publish("data", &payload);

    EXPECT_EQ(received, &payload);
}

TEST(EventBusTest, ResetStopsDelivery) {
    InMemoryEventBus eventBus;
    int calls = 0;
    auto token = eventBus.subscribe("event", [&](const void*) { ++calls; });
    ASSERT_NE(token, nullptr);

    eventBus.publish("event", nullptr);
    token->reset();
    eventBus.publish("event", nullptr);

    EXPECT_EQ(calls, 1);
}

TEST(EventBusTest, RejectsEmptyEventNameAndCallback) {
    InMemoryEventBus eventBus;

    EXPECT_EQ(eventBus.subscribe("", [](const void*) {}), nullptr);
    EXPECT_EQ(eventBus.subscribe("event", {}), nullptr);
}

TEST(EventBusTest, ContinuesAfterCallbackThrows) {
    InMemoryEventBus eventBus;
    int calls = 0;
    auto first = eventBus.subscribe("event", [&](const void*) {
        ++calls;
        throw std::runtime_error("callback failure");
    });
    auto second = eventBus.subscribe("event", [&](const void*) { ++calls; });
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);

    EXPECT_NO_THROW(eventBus.publish("event", nullptr));
    EXPECT_EQ(calls, 2);
}

TEST(CommandBusTest, RegistersAndDispatchesHandler) {
    InMemoryCommandBus commandBus;
    int expected = 42;
    const void* received = nullptr;
    ASSERT_TRUE(commandBus.registerHandler("set", [&](const void* data) {
        received = data;
        return framework::core::Result<void>{};
    }));

    ASSERT_TRUE(commandBus.send("set", &expected));
    EXPECT_EQ(received, &expected);
}

TEST(CommandBusTest, PropagatesHandlerError) {
    InMemoryCommandBus commandBus;
    ASSERT_TRUE(commandBus.registerHandler("fail", [](const void*) {
        return framework::core::Result<void>(
            framework::core::Error(ErrorCode::StateError, "handler failed"));
    }));

    const auto result = commandBus.send("fail", nullptr);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), ErrorCode::StateError);
    EXPECT_EQ(result.error().message(), "handler failed");
}

TEST(CommandBusTest, RejectsInvalidAndDuplicateHandlers) {
    InMemoryCommandBus commandBus;

    EXPECT_EQ(commandBus.registerHandler("", [](const void*) {
                  return framework::core::Result<void>{};
              }).error().code(), ErrorCode::InvalidArgument);
    EXPECT_EQ(commandBus.registerHandler("valid", {}).error().code(), ErrorCode::InvalidArgument);
    ASSERT_TRUE(commandBus.registerHandler("valid", [](const void*) {
        return framework::core::Result<void>{};
    }));
    const auto duplicate = commandBus.registerHandler("valid", [](const void*) {
        return framework::core::Result<void>{};
    });

    ASSERT_FALSE(duplicate);
    EXPECT_EQ(duplicate.error().code(), ErrorCode::AlreadyExists);
}

TEST(CommandBusTest, ReportsMissingHandler) {
    InMemoryCommandBus commandBus;

    const auto result = commandBus.send("missing", nullptr);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), ErrorCode::NotFound);
}

TEST(CommandBusTest, ConvertsHandlerExceptionToInternalError) {
    InMemoryCommandBus commandBus;
    ASSERT_TRUE(commandBus.registerHandler("throws", [](const void*) -> framework::core::Result<void> {
        throw std::runtime_error("handler failure");
    }));

    const auto result = commandBus.send("throws", nullptr);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), ErrorCode::InternalError);
}

TEST(DiagnosticsTest, DefaultProviderReturnsEmptySnapshot) {
    BasicDiagnostics diagnostics;

    const auto snapshot = diagnostics.captureSnapshot();

    EXPECT_TRUE(snapshot.runtimeState.empty());
    EXPECT_TRUE(snapshot.activeModules.empty());
}

TEST(DiagnosticsTest, UsesAndReplacesProvider) {
    BasicDiagnostics diagnostics([] {
        return DiagnosticSnapshot{"Running", {"core", "example"}};
    });

    auto snapshot = diagnostics.captureSnapshot();
    ASSERT_EQ(snapshot.runtimeState, "Running");
    ASSERT_EQ(snapshot.activeModules, (std::vector<std::string>{"core", "example"}));

    diagnostics.setProvider([] { return DiagnosticSnapshot{"Stopped", {}}; });
    snapshot = diagnostics.captureSnapshot();
    EXPECT_EQ(snapshot.runtimeState, "Stopped");
    EXPECT_TRUE(snapshot.activeModules.empty());

    diagnostics.setProvider({});
    snapshot = diagnostics.captureSnapshot();
    EXPECT_TRUE(snapshot.runtimeState.empty());
}

TEST(SchedulerTest, RejectsInvalidIntervalAndTask) {
    ThreadScheduler scheduler;

    EXPECT_EQ(scheduler.scheduleInterval(std::chrono::milliseconds::zero(), [] {}), nullptr);
    EXPECT_EQ(scheduler.scheduleInterval(std::chrono::milliseconds(1), {}), nullptr);
}

TEST(SchedulerTest, ExecutesTaskRepeatedly) {
    ThreadScheduler scheduler;
    std::mutex mutex;
    std::condition_variable condition;
    int calls = 0;
    auto token = scheduler.scheduleInterval(std::chrono::milliseconds(5), [&] {
        std::lock_guard lock(mutex);
        ++calls;
        condition.notify_one();
    });
    ASSERT_NE(token, nullptr);

    std::unique_lock lock(mutex);
    ASSERT_TRUE(condition.wait_for(lock, std::chrono::seconds(1), [&] { return calls >= 2; }));
    token->reset();
    EXPECT_GE(calls, 2);
}

TEST(SchedulerTest, CancellationStopsFutureCallbacks) {
    ThreadScheduler scheduler;
    std::mutex mutex;
    std::condition_variable condition;
    int calls = 0;
    auto token = scheduler.scheduleInterval(std::chrono::milliseconds(5), [&] {
        std::lock_guard lock(mutex);
        ++calls;
        condition.notify_one();
    });
    ASSERT_NE(token, nullptr);
    {
        std::unique_lock lock(mutex);
        ASSERT_TRUE(condition.wait_for(lock, std::chrono::seconds(1), [&] { return calls > 0; }));
    }
    token->reset();
    const int callsAfterReset = calls;
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    EXPECT_EQ(calls, callsAfterReset);
}

TEST(SchedulerTest, ContainsTaskException) {
    ThreadScheduler scheduler;
    std::mutex mutex;
    std::condition_variable condition;
    int calls = 0;
    auto token = scheduler.scheduleInterval(std::chrono::milliseconds(5), [&] {
        std::lock_guard lock(mutex);
        ++calls;
        condition.notify_one();
        throw std::runtime_error("task failure");
    });
    ASSERT_NE(token, nullptr);

    std::unique_lock lock(mutex);
    EXPECT_TRUE(condition.wait_for(lock, std::chrono::seconds(1), [&] { return calls >= 2; }));
    token->reset();
}

TEST(LoggerTest, WritesLevelCategoryAndMessage) {
    std::ostringstream output;
    ConsoleLogger logger(output);

    logger.log(LogLevel::Info, "Test", "message");

    const auto text = output.str();
    EXPECT_NE(text.find("[Info]"), std::string::npos);
    EXPECT_NE(text.find("[Test] message"), std::string::npos);
}

TEST(LoggerTest, WritesAllKnownLevels) {
    std::ostringstream output;
    ConsoleLogger logger(output);

    logger.log(LogLevel::Debug, "Test", "debug");
    logger.log(LogLevel::Warning, "Test", "warning");
    logger.log(LogLevel::Error, "Test", "error");
    logger.log(LogLevel::Fatal, "Test", "fatal");

    const auto text = output.str();
    EXPECT_NE(text.find("[Debug]"), std::string::npos);
    EXPECT_NE(text.find("[Warning]"), std::string::npos);
    EXPECT_NE(text.find("[Error]"), std::string::npos);
    EXPECT_NE(text.find("[Fatal]"), std::string::npos);
}

} // namespace