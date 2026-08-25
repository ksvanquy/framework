#include "runtime/application.h"

#include <gtest/gtest.h>

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

} // namespace