#pragma once

#include "icommand_bus.h"
#include "iconfig.h"
#include "idiagnostics.h"
#include "ievent_bus.h"
#include "ilogger.h"
#include "ischeduler.h"
#include "istorage.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <ostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace framework::services {

class ConsoleLogger final : public ILogger {
public:
    explicit ConsoleLogger(std::ostream& output);
    void log(LogLevel level, std::string_view category, std::string_view message) override;

private:
    std::ostream& output_;
    std::mutex mutex_;
};

class InMemoryConfig final : public IConfig {
public:
    core::Result<std::string> getString(const std::string& key) const override;
    core::Result<void> setString(std::string key, std::string value);

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::string> values_;
};

class InMemoryEventBus final : public IEventBus {
public:
    InMemoryEventBus();
    ~InMemoryEventBus() override;

    void publish(const std::string& eventName, const void* data) override;
    std::unique_ptr<SubscriptionToken> subscribe(
        const std::string& eventName,
        std::function<void(const void*)> callback) override;

private:
    struct State;
    std::shared_ptr<State> state_;
};

class InMemoryCommandBus final : public ICommandBus {
public:
    using Handler = std::function<core::Result<void>(const void*)>;

    core::Result<void> registerHandler(std::string commandName, Handler handler);
    core::Result<void> send(const std::string& commandName, const void* data) override;

private:
    std::mutex mutex_;
    std::unordered_map<std::string, Handler> handlers_;
};

class InMemoryStorage final : public IStorage {
public:
    core::Result<void> set(const std::string& key, const std::string& value) override;
    core::Result<std::string> get(const std::string& key) override;

private:
    std::mutex mutex_;
    std::unordered_map<std::string, std::string> values_;
};

class ThreadScheduler final : public IScheduler {
public:
    ~ThreadScheduler() override;

    std::unique_ptr<SubscriptionToken> scheduleInterval(
        std::chrono::milliseconds interval,
        std::function<void()> task) override;

private:
    struct Job {
        std::atomic_bool stop = false;
        std::thread worker;

        ~Job() {
            stop.store(true);
            if (worker.joinable()) {
                worker.join();
            }
        }
    };

    std::mutex mutex_;
    std::vector<std::shared_ptr<Job>> jobs_;
};

class BasicDiagnostics final : public IDiagnostics {
public:
    using SnapshotProvider = std::function<DiagnosticSnapshot()>;

    explicit BasicDiagnostics(SnapshotProvider provider = {});
    DiagnosticSnapshot captureSnapshot() const override;
    void setProvider(SnapshotProvider provider);

private:
    mutable std::mutex mutex_;
    SnapshotProvider provider_;
};

} // namespace framework::services
