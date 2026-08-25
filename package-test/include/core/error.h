#pragma once

#include <string>
#include <source_location>
#include <utility>

namespace framework::core {

enum class ErrorCode {
    Success = 0,
    InvalidArgument,
    NotFound,
    AlreadyExists,
    Timeout,
    StateError,
    PluginLoadFailed,
    InternalError
};

class Error {
public:
    Error(ErrorCode code,
          std::string message,
          std::source_location loc = std::source_location::current())
        : code_(code), message_(std::move(message)), location_(loc) {}

    [[nodiscard]] ErrorCode code() const noexcept { return code_; }
    [[nodiscard]] const std::string& message() const noexcept { return message_; }
    [[nodiscard]] const std::source_location& location() const noexcept { return location_; }
    [[nodiscard]] std::string toString() const {
        return "[" + std::to_string(static_cast<int>(code_)) + "] " + message_ + 
               " (at " + location_.file_name() + ":" + std::to_string(location_.line()) + ")";
    }

private:
    ErrorCode code_;
    std::string message_;
    std::source_location location_;
};

} // namespace framework::core