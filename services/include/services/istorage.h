#pragma once
#include <string>
#include "core/result.h"

namespace framework::services {
class IStorage {
public:
    virtual ~IStorage() = default;
    virtual core::Result<void> set(const std::string& key, const std::string& value) = 0;
    virtual core::Result<std::string> get(const std::string& key) = 0;
};
} // namespace framework::services