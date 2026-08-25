#include "core/result.h"

#include <gtest/gtest.h>

TEST(ResultStressTest, RepeatedValueAndErrorConstruction) {
    constexpr int iterations = 10000;

    for (int index = 0; index < iterations; ++index) {
        const framework::core::Result<int> success(index);
        ASSERT_TRUE(success);
        ASSERT_EQ(success.value(), index);

        const framework::core::Result<int> failure(
            framework::core::Error(framework::core::ErrorCode::InternalError, "stress"));
        ASSERT_FALSE(failure);
        ASSERT_EQ(failure.error().code(), framework::core::ErrorCode::InternalError);
    }
}