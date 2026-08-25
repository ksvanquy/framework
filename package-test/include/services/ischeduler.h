#pragma once
#include <functional>
#include <chrono>
#include <memory>
#include "ievent_bus.h"

namespace framework::services {
class IScheduler {
public:
    virtual ~IScheduler() = default;
    virtual std::unique_ptr<SubscriptionToken> scheduleInterval(
        std::chrono::milliseconds interval,
        std::function<void()> task) = 0;
};
} // namespace framework::services