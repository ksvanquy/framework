#pragma once

#include "core/error.h"
#include "core/result.h"

#include <string>

namespace framework::tests {

inline core::Result<int> successfulIntResult() {
    return 42;
}

inline core::Result<int> failedIntResult() {
    return core::Error(core::ErrorCode::InvalidArgument, "invalid test value");
}

inline core::Result<void> successfulVoidResult() {
    return {};
}

inline core::Result<void> failedVoidResult() {
    return core::Error(core::ErrorCode::StateError, "invalid test state");
}

} // namespace framework::tests