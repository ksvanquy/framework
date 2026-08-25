#include "fixtures/core_test_helpers.h"

#include <gtest/gtest.h>

#include <string>

namespace {

TEST(ResultTest, StoresValue) {
    const auto result = framework::tests::successfulIntResult();

    ASSERT_TRUE(result.hasValue());
    ASSERT_TRUE(static_cast<bool>(result));
    EXPECT_EQ(result.value(), 42);
}

TEST(ResultTest, StoresError) {
    const auto result = framework::tests::failedIntResult();

    ASSERT_FALSE(result.hasValue());
    ASSERT_FALSE(static_cast<bool>(result));
    EXPECT_EQ(result.error().code(), framework::core::ErrorCode::InvalidArgument);
    EXPECT_EQ(result.error().message(), "invalid test value");
}

TEST(ResultTest, SupportsMoveOnlyValue) {
    framework::core::Result<std::string> result(std::string("value"));

    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), "value");
}

TEST(ResultTest, VoidSuccessHasNoError) {
    const auto result = framework::tests::successfulVoidResult();

    EXPECT_TRUE(result.hasValue());
    EXPECT_TRUE(static_cast<bool>(result));
}

TEST(ResultTest, VoidFailureStoresError) {
    const auto result = framework::tests::failedVoidResult();

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), framework::core::ErrorCode::StateError);
}

} // namespace