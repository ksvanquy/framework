#include "runtime/application.h"
#include "runtime/imodule.h"
#include "runtime/module_manager.h"

#include <gtest/gtest.h>

#include <memory>

namespace {

class SuccessfulApplication final : public framework::runtime::Application {
protected:
    framework::core::Result<void> onConfigureModules(framework::runtime::Runtime&) override {
        return {};
    }

    int onRun() override { return 7; }
};

class ConfigureFailureApplication final : public framework::runtime::Application {
protected:
    framework::core::Result<void> onConfigureModules(framework::runtime::Runtime&) override {
        return framework::core::Error(
            framework::core::ErrorCode::InvalidArgument, "invalid application configuration");
    }
};

class StopTrackingModule final : public framework::runtime::IModule {
public:
    explicit StopTrackingModule(bool& stopped) : stopped_(stopped) {}

    const framework::runtime::ModuleInfo& info() const override { return info_; }
    framework::runtime::ModuleState state() const override { return state_; }

    framework::core::Result<void> initialize() override {
        state_ = framework::runtime::ModuleState::Initialized;
        return {};
    }

    framework::core::Result<void> start() override {
        state_ = framework::runtime::ModuleState::Started;
        return {};
    }

    framework::core::Result<void> stop() override {
        stopped_ = true;
        state_ = framework::runtime::ModuleState::Stopped;
        return {};
    }

private:
    framework::runtime::ModuleInfo info_{
        "application.stop.tracker", "Application stop tracker", "1.0.0", {}};
    bool& stopped_;
    framework::runtime::ModuleState state_ = framework::runtime::ModuleState::Discovered;
};

class RunFailureApplication final : public framework::runtime::Application {
public:
    explicit RunFailureApplication(bool& stopped) : stopped_(stopped) {}

protected:
    framework::core::Result<void> onConfigureModules(framework::runtime::Runtime& runtime) override {
        return runtime.moduleManager().registerModule(
            std::make_unique<StopTrackingModule>(stopped_));
    }

    int onRun() override { return 23; }

private:
    bool& stopped_;
};

enum class FailurePoint { Initialize, Start, Stop };

class FailingLifecycleModule final : public framework::runtime::IModule {
public:
    FailingLifecycleModule(FailurePoint failurePoint, bool& stopCalled)
        : failurePoint_(failurePoint), stopCalled_(stopCalled) {}

    const framework::runtime::ModuleInfo& info() const override { return info_; }
    framework::runtime::ModuleState state() const override { return state_; }

    framework::core::Result<void> initialize() override {
        if (failurePoint_ == FailurePoint::Initialize) {
            return framework::core::Error(
                framework::core::ErrorCode::InternalError, "application initialize failed");
        }
        state_ = framework::runtime::ModuleState::Initialized;
        return {};
    }

    framework::core::Result<void> start() override {
        if (failurePoint_ == FailurePoint::Start) {
            return framework::core::Error(
                framework::core::ErrorCode::InternalError, "application start failed");
        }
        state_ = framework::runtime::ModuleState::Started;
        return {};
    }

    framework::core::Result<void> stop() override {
        stopCalled_ = true;
        if (failurePoint_ == FailurePoint::Stop) {
            return framework::core::Error(
                framework::core::ErrorCode::InternalError, "application stop failed");
        }
        state_ = framework::runtime::ModuleState::Stopped;
        return {};
    }

private:
    framework::runtime::ModuleInfo info_{"application.failing", "Application failing module", "1.0.0", {}};
    FailurePoint failurePoint_;
    bool& stopCalled_;
    framework::runtime::ModuleState state_ = framework::runtime::ModuleState::Discovered;
};

class LifecycleFailureApplication final : public framework::runtime::Application {
public:
    LifecycleFailureApplication(FailurePoint failurePoint, bool& stopCalled)
        : failurePoint_(failurePoint), stopCalled_(stopCalled) {}

protected:
    framework::core::Result<void> onConfigureModules(framework::runtime::Runtime& runtime) override {
        return runtime.moduleManager().registerModule(
            std::make_unique<FailingLifecycleModule>(failurePoint_, stopCalled_));
    }

private:
    FailurePoint failurePoint_;
    bool& stopCalled_;
};

TEST(ApplicationIntegrationTest, RunsAndReturnsApplicationExitCode) {
    SuccessfulApplication application;

    EXPECT_EQ(application.exec(0, nullptr), 7);
    EXPECT_EQ(application.lastError(), nullptr);
}

TEST(ApplicationIntegrationTest, PropagatesConfigureFailure) {
    ConfigureFailureApplication application;

    EXPECT_EQ(application.exec(0, nullptr), -1);
    ASSERT_NE(application.lastError(), nullptr);
    EXPECT_EQ(application.lastError()->code(), framework::core::ErrorCode::InvalidArgument);
}

TEST(ApplicationIntegrationTest, StopsRuntimeWhenRunFails) {
    bool stopped = false;
    RunFailureApplication application(stopped);

    EXPECT_EQ(application.exec(0, nullptr), 23);
    EXPECT_TRUE(stopped);
    EXPECT_EQ(application.lastError(), nullptr);
}

TEST(ApplicationIntegrationTest, PropagatesInitializeFailure) {
    bool stopCalled = false;
    LifecycleFailureApplication application(FailurePoint::Initialize, stopCalled);

    EXPECT_EQ(application.exec(0, nullptr), -2);
    ASSERT_NE(application.lastError(), nullptr);
    EXPECT_EQ(application.lastError()->code(), framework::core::ErrorCode::InternalError);
    EXPECT_FALSE(stopCalled);
}

TEST(ApplicationIntegrationTest, PropagatesStartFailure) {
    bool stopCalled = false;
    LifecycleFailureApplication application(FailurePoint::Start, stopCalled);

    EXPECT_EQ(application.exec(0, nullptr), -3);
    ASSERT_NE(application.lastError(), nullptr);
    EXPECT_EQ(application.lastError()->code(), framework::core::ErrorCode::InternalError);
    EXPECT_FALSE(stopCalled);
}

TEST(ApplicationIntegrationTest, PropagatesStopFailure) {
    bool stopCalled = false;
    LifecycleFailureApplication application(FailurePoint::Stop, stopCalled);

    EXPECT_EQ(application.exec(0, nullptr), -4);
    ASSERT_NE(application.lastError(), nullptr);
    EXPECT_EQ(application.lastError()->code(), framework::core::ErrorCode::InternalError);
    EXPECT_TRUE(stopCalled);
}

} // namespace