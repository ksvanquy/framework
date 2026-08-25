#include "runtime_bridge.h"

#include "example_module/example_module.h"
#include "runtime/module_manager.h"

namespace framework::ui {

RuntimeBridge::RuntimeBridge(QObject* parent)
        : QObject(parent),
            runtime_(std::make_unique<runtime::Runtime>()) {}

RuntimeBridge::~RuntimeBridge() {
        runtime_->stop();
}

QString RuntimeBridge::state() const {
    return state_;
}

QString RuntimeBridge::lastError() const {
    return lastError_;
}

bool RuntimeBridge::moduleRegistered() const {
    return moduleRegistered_;
}

void RuntimeBridge::start() {
    if (!moduleRegistered_) {
        auto result = runtime_->moduleManager().registerModule(
            std::make_unique<modules::ExampleModule>(runtime_->logger(), runtime_->eventBus()));
        if (!result) {
            setError(QString::fromStdString(result.error().message()));
            return;
        }
        moduleRegistered_ = true;
        emit moduleRegisteredChanged();
    }

    auto result = runtime_->initialize();
    if (!result) {
        setError(QString::fromStdString(result.error().message()));
        return;
    }
    result = runtime_->start();
    if (!result) {
        setError(QString::fromStdString(result.error().message()));
        return;
    }
    clearError();
    setState(QStringLiteral("Running"));
}

void RuntimeBridge::stop() {
    const auto result = runtime_->stop();
    if (!result) {
        setError(QString::fromStdString(result.error().message()));
        return;
    }
    clearError();
    setState(QStringLiteral("Stopped"));
}

void RuntimeBridge::reset() {
    runtime_->stop();
    runtime_ = std::make_unique<runtime::Runtime>();
    moduleRegistered_ = false;
    clearError();
    setState(QStringLiteral("Stopped"));
    emit moduleRegisteredChanged();
}

void RuntimeBridge::clearError() {
    setError({});
}

void RuntimeBridge::setError(QString message) {
    if (lastError_ == message) {
        return;
    }
    lastError_ = std::move(message);
    emit lastErrorChanged();
    if (!lastError_.isEmpty()) {
        emit errorOccurred(lastError_);
    }
}

void RuntimeBridge::setState(QString state) {
    if (state_ == state) {
        return;
    }
    state_ = std::move(state);
    emit stateChanged();
}

} // namespace framework::ui