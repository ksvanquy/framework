#include "core/error.h"

#include <gtest/gtest.h>

#include <string>

namespace {

TEST(ErrorTest, PreservesCodeAndMessage) {
    const framework::core::Error error(
        framework::core::ErrorCode::NotFound, "item is missing");

    EXPECT_EQ(error.code(), framework::core::ErrorCode::NotFound);
    EXPECT_EQ(error.message(), "item is missing");
}

TEST(ErrorTest, CapturesSourceLocation) {
    const framework::core::Error error(
        framework::core::ErrorCode::InternalError, "failure");

    EXPECT_FALSE(error.location().file_name()[0] == '\0');
    EXPECT_GT(error.location().line(), 0U);
    EXPECT_NE(error.toString().find("failure"), std::string::npos);
}

} // namespace