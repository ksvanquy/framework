#pragma once
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include "imodule.h"
#include "plugin_loader.h"
#include "core/result.h"
#include "services/ilogger.h"

namespace framework::runtime {

class ModuleManager {
public:
    explicit ModuleManager(services::ILogger& logger);
    ~ModuleManager() = default;

    // Registration accepts only Discovered or Loaded modules.
    core::Result<void> registerModule(std::unique_ptr<IModule> module);
    core::Result<void> registerPlugin(LoadedPlugin plugin);
    core::Result<void> unloadPlugin(const std::string& moduleId);
    core::Result<void> unloadAllPlugins();

    // Operations are dependency ordered: initialize/start forward, stop reverse.
    core::Result<void> initializeAll();
    core::Result<void> startAll();
    core::Result<void> stopAll();

    [[nodiscard]] ModuleState getModuleState(const std::string& moduleId) const;

private:
    services::ILogger& logger_;
    std::unordered_map<std::string, std::shared_ptr<LoadedPlugin>> pluginOwners_;
    std::unordered_map<std::string, std::unique_ptr<IModule>> modules_;
    std::vector<std::string> initializationOrder_;

    core::Result<void> resolveDependenciesAndSort();
};

} // namespace framework::runtime