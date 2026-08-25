#include "core/error.h"
#include "core/interfaces.h"
#include "core/module_id.h"
#include "core/result.h"
#include "core/types.h"

#include <gtest/gtest.h>

#include <utility>

TEST(CoreSystemTest, CoreHeadersAreUsableTogether) {
    framework::core::ByteBuffer payload{0x01, 0x02};
    framework::core::Result<framework::core::ByteBuffer> result(std::move(payload));

    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().size(), 2U);
}