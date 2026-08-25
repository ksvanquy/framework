#pragma once
#include "runtime.h"

#include <optional>

namespace framework::runtime {

class Application {
public:
    Application() = default;
    virtual ~Application() = default;

    int exec(int argc, char* argv[]) {
        (void)argc;
        (void)argv;
        lastError_.reset();

        auto result = onConfigureModules(runtime_);
        if (!result) return fail(-1, result.error());
        result = runtime_.initialize();
        if (!result) return fail(-2, result.error());
        result = runtime_.start();
        if (!result) return fail(-3, result.error());

        int exitCode = onRun();
        result = runtime_.stop();
        if (!result) return fail(exitCode == 0 ? -4 : exitCode, result.error());
        return exitCode;
    }

    [[nodiscard]] const core::Error* lastError() const noexcept {
        return lastError_ ? &*lastError_ : nullptr;
    }

protected:
    virtual core::Result<void> onConfigureModules(Runtime& runtime) = 0;
    virtual int onRun() { return 0; }

    Runtime runtime_;

private:
    int fail(int exitCode, const core::Error& error) {
        lastError_ = error;
        return exitCode;
    }

    std::optional<core::Error> lastError_;
};

} // namespace framework::runtime