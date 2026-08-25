#pragma once
#include <string>
#include "core/result.h"

namespace framework::services {
class IConfig {
public:
    virtual ~IConfig() = default;
    virtual core::Result<std::string> getString(const std::string& key) const = 0;
};
} // namespace framework::services