#include "runtime_bridge.h"

#include "example_module/example_module.h"
#include "runtime/module_manager.h"

namespace framework::ui {

RuntimeBridge::RuntimeBridge(QObject* parent) : QObject(parent) {}

RuntimeBridge::~RuntimeBridge() {
    runtime_.stop();
}

QString RuntimeBridge::state() const {
    return state_;
}

void RuntimeBridge::start() {
    if (!moduleRegistered_) {
        auto result = runtime_.moduleManager().registerModule(
            std::make_unique<modules::ExampleModule>(runtime_.logger(), runtime_.eventBus()));
        if (!result) {
            emit errorOccurred(QString::fromStdString(result.error().message()));
            return;
        }
        moduleRegistered_ = true;
    }

    auto result = runtime_.initialize();
    if (!result) {
        emit errorOccurred(QString::fromStdString(result.error().message()));
        return;
    }
    result = runtime_.start();
    if (!result) {
        emit errorOccurred(QString::fromStdString(result.error().message()));
        return;
    }
    setState(QStringLiteral("Running"));
}

void RuntimeBridge::stop() {
    const auto result = runtime_.stop();
    if (!result) {
        emit errorOccurred(QString::fromStdString(result.error().message()));
        return;
    }
    setState(QStringLiteral("Stopped"));
}

void RuntimeBridge::setState(QString state) {
    if (state_ == state) {
        return;
    }
    state_ = std::move(state);
    emit stateChanged();
}

} // namespace framework::ui