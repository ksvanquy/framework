#pragma once

#include "error.h"
#include <optional>
#include <utility>
#include <variant>

namespace framework::core {

template <typename T>
class Result {
public:
    Result(const T& value) : data_(value) {}
    Result(T&& value) : data_(std::move(value)) {}
    Result(Error error) : data_(std::move(error)) {}

    [[nodiscard]] bool hasValue() const noexcept {
        return std::holds_alternative<T>(data_);
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return hasValue();
    }

    [[nodiscard]] const T& value() const& {
        return std::get<T>(data_);
    }

    T& value() & {
        return std::get<T>(data_);
    }

    [[nodiscard]] const Error& error() const& {
        return std::get<Error>(data_);
    }

private:
    std::variant<T, Error> data_;
};

template <>
class Result<void> {
public:
    Result() : error_(std::nullopt) {}
    Result(Error error) : error_(std::move(error)) {}

    [[nodiscard]] bool hasValue() const noexcept {
        return !error_.has_value();
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return hasValue();
    }

    [[nodiscard]] const Error& error() const& {
        return error_.value();
    }

private:
    std::optional<Error> error_;
};

} // namespace framework::core