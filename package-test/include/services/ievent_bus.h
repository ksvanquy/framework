#pragma once
#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace framework::services {

class SubscriptionToken {
public:
    explicit SubscriptionToken(std::function<void()> unsubscribe) 
        : unsubscribe_(std::move(unsubscribe)) {}
    
    ~SubscriptionToken() { reset(); }

    SubscriptionToken(const SubscriptionToken&) = delete;
    SubscriptionToken& operator=(const SubscriptionToken&) = delete;
    SubscriptionToken(SubscriptionToken&&) noexcept = default;
    SubscriptionToken& operator=(SubscriptionToken&& other) noexcept {
        if (this != &other) {
            reset();
            unsubscribe_ = std::move(other.unsubscribe_);
        }
        return *this;
    }

    void reset() noexcept {
        if (unsubscribe_) {
            try {
                unsubscribe_();
            } catch (...) {
            }
            unsubscribe_ = nullptr;
        }
    }

private:
    std::function<void()> unsubscribe_;
};

class IEventBus {
public:
    virtual ~IEventBus() = default;
    virtual void publish(const std::string& eventName, const void* data) = 0;
    virtual std::unique_ptr<SubscriptionToken> subscribe(
        const std::string& eventName, 
        std::function<void(const void*)> callback) = 0;
};

} // namespace framework::services