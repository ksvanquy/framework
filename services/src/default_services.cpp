#include "services/default_services.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <utility>
#include <vector>

namespace framework::services {
namespace {

core::Error makeError(core::ErrorCode code, std::string message) {
    return core::Error(code, std::move(message));
}

const char* levelName(LogLevel level) noexcept {
    switch (level) {
    case LogLevel::Debug: return "Debug";
    case LogLevel::Info: return "Info";
    case LogLevel::Warning: return "Warning";
    case LogLevel::Error: return "Error";
    case LogLevel::Fatal: return "Fatal";
    }
    return "Unknown";
}

} // namespace

ConsoleLogger::ConsoleLogger(std::ostream& output) : output_(output) {}

void ConsoleLogger::log(LogLevel level, std::string_view category, std::string_view message) {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};

#if defined(_WIN32)
    localtime_s(&localTime, &time);
#else
    localtime_r(&time, &localTime);
#endif

    std::lock_guard lock(mutex_);
    output_ << '[' << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S") << "] ["
            << levelName(level) << "] [" << category << "] " << message << '\n';
}

core::Result<std::string> InMemoryConfig::getString(const std::string& key) const {
    std::lock_guard lock(mutex_);
    const auto found = values_.find(key);
    if (found == values_.end()) {
        return makeError(core::ErrorCode::NotFound, "Configuration key not found: " + key);
    }
    return found->second;
}

core::Result<void> InMemoryConfig::setString(std::string key, std::string value) {
    if (key.empty()) {
        return makeError(core::ErrorCode::InvalidArgument, "Configuration key is empty");
    }
    std::lock_guard lock(mutex_);
    values_[std::move(key)] = std::move(value);
    return {};
}

struct InMemoryEventBus::State {
    std::mutex mutex;
    std::size_t nextId = 0;
    std::unordered_map<std::string, std::unordered_map<std::size_t, std::function<void(const void*)>>> subscriptions;
};

InMemoryEventBus::InMemoryEventBus() : state_(std::make_shared<State>()) {}
InMemoryEventBus::~InMemoryEventBus() = default;

void InMemoryEventBus::publish(const std::string& eventName, const void* data) {
    std::vector<std::function<void(const void*)>> callbacks;
    {
        std::lock_guard lock(state_->mutex);
        const auto found = state_->subscriptions.find(eventName);
        if (found == state_->subscriptions.end()) {
            return;
        }
        for (const auto& [id, callback] : found->second) {
            callbacks.push_back(callback);
        }
    }

    for (const auto& callback : callbacks) {
        try {
            callback(data);
        } catch (...) {
        }
    }
}

std::unique_ptr<SubscriptionToken> InMemoryEventBus::subscribe(
    const std::string& eventName, std::function<void(const void*)> callback) {
    if (eventName.empty() || !callback) {
        return nullptr;
    }

    const auto state = state_;
    std::size_t id;
    {
        std::lock_guard lock(state->mutex);
        id = state->nextId++;
        state->subscriptions[eventName].emplace(id, std::move(callback));
    }

    return std::make_unique<SubscriptionToken>([state, eventName, id] {
        std::lock_guard lock(state->mutex);
        const auto found = state->subscriptions.find(eventName);
        if (found == state->subscriptions.end()) {
            return;
        }
        found->second.erase(id);
        if (found->second.empty()) {
            state->subscriptions.erase(found);
        }
    });
}

core::Result<void> InMemoryCommandBus::registerHandler(std::string commandName, Handler handler) {
    if (commandName.empty() || !handler) {
        return makeError(core::ErrorCode::InvalidArgument, "Invalid command handler");
    }
    std::lock_guard lock(mutex_);
    if (handlers_.find(commandName) != handlers_.end()) {
        return makeError(core::ErrorCode::AlreadyExists, "Command handler already exists: " + commandName);
    }
    handlers_.emplace(std::move(commandName), std::move(handler));
    return {};
}

core::Result<void> InMemoryCommandBus::send(const std::string& commandName, const void* data) {
    Handler handler;
    {
        std::lock_guard lock(mutex_);
        const auto found = handlers_.find(commandName);
        if (found == handlers_.end()) {
            return makeError(core::ErrorCode::NotFound, "Command handler not found: " + commandName);
        }
        handler = found->second;
    }
    try {
        return handler(data);
    } catch (...) {
        return makeError(core::ErrorCode::InternalError, "Command handler threw an exception");
    }
}

core::Result<void> InMemoryStorage::set(const std::string& key, const std::string& value) {
    if (key.empty()) {
        return makeError(core::ErrorCode::InvalidArgument, "Storage key is empty");
    }
    std::lock_guard lock(mutex_);
    values_[key] = value;
    return {};
}

core::Result<std::string> InMemoryStorage::get(const std::string& key) {
    std::lock_guard lock(mutex_);
    const auto found = values_.find(key);
    if (found == values_.end()) {
        return makeError(core::ErrorCode::NotFound, "Storage key not found: " + key);
    }
    return found->second;
}

ThreadScheduler::~ThreadScheduler() {
    std::lock_guard lock(mutex_);
    for (const auto& job : jobs_) {
        job->stop.store(true);
    }
}

std::unique_ptr<SubscriptionToken> ThreadScheduler::scheduleInterval(
    std::chrono::milliseconds interval, std::function<void()> task) {
    if (interval <= std::chrono::milliseconds::zero() || !task) {
        return nullptr;
    }

    const auto job = std::make_shared<Job>();
    {
        std::lock_guard lock(mutex_);
        jobs_.push_back(job);
    }
    job->worker = std::thread([job, interval, task = std::move(task)] {
        while (!job->stop.load()) {
            std::this_thread::sleep_for(interval);
            if (job->stop.load()) {
                break;
            }
            try {
                task();
            } catch (...) {
            }
        }
    });

    return std::make_unique<SubscriptionToken>([job] {
        job->stop.store(true);
        if (job->worker.joinable()) {
            job->worker.join();
        }
    });
}

BasicDiagnostics::BasicDiagnostics(SnapshotProvider provider) : provider_(std::move(provider)) {}

DiagnosticSnapshot BasicDiagnostics::captureSnapshot() const {
    SnapshotProvider provider;
    {
        std::lock_guard lock(mutex_);
        provider = provider_;
    }
    return provider ? provider() : DiagnosticSnapshot{};
}

void BasicDiagnostics::setProvider(SnapshotProvider provider) {
    std::lock_guard lock(mutex_);
    provider_ = std::move(provider);
}

} // namespace framework::services
