#include "core/error.h"
#include "core/interfaces.h"
#include "core/module_id.h"
#include "core/result.h"
#include "core/types.h"

#include <gtest/gtest.h>

TEST(CoreIntegrationTest, PublicTypesComposeWithoutUpperLayers) {
    const framework::core::ModuleId moduleId = "example.module";
    const framework::core::ServiceId serviceId = "example.service";
    const framework::core::ByteBuffer bytes{1, 2, 3};

    EXPECT_EQ(moduleId, "example.module");
    EXPECT_EQ(serviceId, "example.service");
    EXPECT_EQ(bytes.size(), 3U);
}