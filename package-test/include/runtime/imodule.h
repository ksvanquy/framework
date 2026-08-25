#pragma once
#include <string>
#include <vector>
#include "core/result.h"

namespace framework::runtime {

enum class ModuleState {
    Discovered,
    Loaded,
    Initialized,
    Started,
    Running,
    Stopping,
    Stopped,
    Unloaded
};

[[nodiscard]] constexpr bool canInitialize(ModuleState state) noexcept {
    return state == ModuleState::Discovered || state == ModuleState::Loaded;
}

[[nodiscard]] constexpr bool canStart(ModuleState state) noexcept {
    return state == ModuleState::Initialized;
}

[[nodiscard]] constexpr bool canStop(ModuleState state) noexcept {
    return state == ModuleState::Started || state == ModuleState::Running;
}

struct ModuleInfo {
    std::string id;
    std::string name;
    std::string version;
    std::vector<std::string> dependencies;
};

class IModule {
public:
    virtual ~IModule() = default;
    virtual const ModuleInfo& info() const = 0;
    virtual ModuleState state() const = 0;

    // Implementations own their state and must commit the transition before returning success.
    virtual core::Result<void> initialize() = 0;
    virtual core::Result<void> start() = 0;
    virtual core::Result<void> stop() = 0;
};

} // namespace framework::runtime