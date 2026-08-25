#pragma once
#include <string>
#include "core/result.h"

namespace framework::services {
class ICommandBus {
public:
    virtual ~ICommandBus() = default;
    virtual core::Result<void> send(const std::string& commandName, const void* data) = 0;
};
} // namespace framework::services