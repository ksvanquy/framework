#pragma once
#include <string_view>

namespace framework::services {

enum class LogLevel { Debug, Info, Warning, Error, Fatal };

class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void log(LogLevel level, std::string_view category, std::string_view message) = 0;
};

} // namespace framework::services