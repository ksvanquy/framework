#include "runtime/module_manager.h"

#include <algorithm>
#include <functional>
#include <utility>

namespace framework::runtime {
namespace {

class PluginModuleProxy final : public IModule {
public:
    PluginModuleProxy(IModule& module, const PluginDescriptor& descriptor)
        : module_(module), info_(module.info()) {
        info_.id = descriptor.id;
        info_.name = descriptor.name;
        info_.version = std::to_string(descriptor.version.major) + "." +
            std::to_string(descriptor.version.minor) + "." +
            std::to_string(descriptor.version.patch);
        info_.dependencies.reserve(descriptor.dependencyCount);
        for (uint32_t index = 0; index < descriptor.dependencyCount; ++index) {
            info_.dependencies.emplace_back(descriptor.dependencies[index]);
        }
    }

    const ModuleInfo& info() const override { return info_; }
    ModuleState state() const override { return module_.state(); }
    core::Result<void> initialize() override { return module_.initialize(); }
    core::Result<void> start() override { return module_.start(); }
    core::Result<void> stop() override { return module_.stop(); }

private:
    IModule& module_;
    ModuleInfo info_;
};

core::Result<void> makeError(core::ErrorCode code, std::string message) {
    return core::Error(code, std::move(message));
}

const char* stateName(ModuleState state) noexcept {
    switch (state) {
    case ModuleState::Discovered: return "Discovered";
    case ModuleState::Loaded: return "Loaded";
    case ModuleState::Initialized: return "Initialized";
    case ModuleState::Started: return "Started";
    case ModuleState::Running: return "Running";
    case ModuleState::Stopping: return "Stopping";
    case ModuleState::Stopped: return "Stopped";
    case ModuleState::Unloaded: return "Unloaded";
    }
    return "Unknown";
}

} // namespace

ModuleManager::ModuleManager(services::ILogger& logger) : logger_(logger) {}

core::Result<void> ModuleManager::registerModule(std::unique_ptr<IModule> module) {
    if (!module) {
        return makeError(core::ErrorCode::InvalidArgument, "Cannot register a null module");
    }

    const ModuleInfo& moduleInfo = module->info();
    if (moduleInfo.id.empty()) {
        return makeError(core::ErrorCode::InvalidArgument, "Module ID is empty");
    }
    if (modules_.find(moduleInfo.id) != modules_.end()) {
        return makeError(core::ErrorCode::AlreadyExists, "Module already registered: " + moduleInfo.id);
    }
    if (!canInitialize(module->state())) {
        return makeError(core::ErrorCode::StateError, "Module must be Discovered or Loaded: " + moduleInfo.id);
    }

    logger_.log(services::LogLevel::Debug, "Runtime.ModuleManager", "Registering module: " + moduleInfo.id);
    modules_.emplace(moduleInfo.id, std::move(module));
    initializationOrder_.clear();
    return {};
}

core::Result<void> ModuleManager::registerPlugin(LoadedPlugin plugin) {
    const auto& descriptor = plugin.descriptor();
    const std::string id = descriptor.id;
    if (modules_.find(id) != modules_.end()) {
        return makeError(core::ErrorCode::AlreadyExists, "Module already registered: " + id);
    }
    if (!canInitialize(plugin.module().state())) {
        return makeError(core::ErrorCode::StateError, "Plugin module must be Discovered or Loaded: " + id);
    }

    auto owner = std::make_shared<LoadedPlugin>(std::move(plugin));
    auto proxy = std::make_unique<PluginModuleProxy>(owner->module(), owner->descriptor());
    logger_.log(services::LogLevel::Debug, "Runtime.ModuleManager", "Registering plugin module: " + id);
    modules_.emplace(id, std::move(proxy));
    pluginOwners_.emplace(id, std::move(owner));
    initializationOrder_.clear();
    return {};
}

core::Result<void> ModuleManager::unloadPlugin(const std::string& moduleId) {
    const auto owner = pluginOwners_.find(moduleId);
    if (owner == pluginOwners_.end()) {
        return makeError(core::ErrorCode::NotFound, "Plugin module not found: " + moduleId);
    }

    const auto module = modules_.find(moduleId);
    if (module == modules_.end()) {
        pluginOwners_.erase(owner);
        return {};
    }

    const ModuleState currentState = module->second->state();
    if (canStop(currentState)) {
        auto result = module->second->stop();
        if (!result) {
            return result;
        }
        if (module->second->state() != ModuleState::Stopped) {
            return makeError(core::ErrorCode::StateError,
                             "Plugin did not enter Stopped state: " + moduleId);
        }
    } else if (currentState != ModuleState::Discovered &&
               currentState != ModuleState::Loaded &&
               currentState != ModuleState::Initialized &&
               currentState != ModuleState::Stopped) {
        return makeError(core::ErrorCode::StateError,
                         "Plugin cannot be unloaded from state " +
                         std::string(stateName(currentState)) + ": " + moduleId);
    }

    logger_.log(services::LogLevel::Debug, "Runtime.ModuleManager", "Unloading plugin module: " + moduleId);
    modules_.erase(module);
    pluginOwners_.erase(owner);
    initializationOrder_.clear();
    return {};
}

core::Result<void> ModuleManager::unloadAllPlugins() {
    std::vector<std::string> pluginIds;
    pluginIds.reserve(pluginOwners_.size());
    for (const auto& [id, owner] : pluginOwners_) {
        pluginIds.push_back(id);
    }

    core::Result<void> firstError;
    bool hasError = false;
    for (const auto& id : pluginIds) {
        auto result = unloadPlugin(id);
        if (!result && !hasError) {
            firstError = result;
            hasError = true;
        }
    }
    return hasError ? firstError : core::Result<void>{};
}

core::Result<void> ModuleManager::resolveDependenciesAndSort() {
    initializationOrder_.clear();

    std::vector<std::string> moduleIds;
    moduleIds.reserve(modules_.size());
    for (const auto& [id, module] : modules_) {
        moduleIds.push_back(id);
        for (const auto& dependency : module->info().dependencies) {
            if (modules_.find(dependency) == modules_.end()) {
                return makeError(core::ErrorCode::NotFound,
                                 "Dependency '" + dependency + "' required by module '" + id + "' was not found");
            }
        }
    }
    std::sort(moduleIds.begin(), moduleIds.end());

    enum class VisitState { Unvisited, Visiting, Visited };
    std::unordered_map<std::string, VisitState> visitStates;
    std::vector<std::string> path;

    std::function<core::Result<void>(const std::string&)> visit =
        [&](const std::string& id) -> core::Result<void> {
        auto& visitState = visitStates[id];
        if (visitState == VisitState::Visited) {
            return {};
        }
        if (visitState == VisitState::Visiting) {
            const auto cycleStart = std::find(path.begin(), path.end(), id);
            std::string cycle;
            for (auto current = cycleStart; current != path.end(); ++current) {
                if (!cycle.empty()) cycle += " -> ";
                cycle += *current;
            }
            if (!cycle.empty()) cycle += " -> ";
            cycle += id;
            logger_.log(services::LogLevel::Error, "Runtime.ModuleManager", "Circular dependency: " + cycle);
            return makeError(core::ErrorCode::StateError, "Circular module dependency: " + cycle);
        }

        visitState = VisitState::Visiting;
        path.push_back(id);
        for (const auto& dependency : modules_.at(id)->info().dependencies) {
            auto result = visit(dependency);
            if (!result) {
                return result;
            }
        }
        path.pop_back();
        visitState = VisitState::Visited;
        initializationOrder_.push_back(id);
        return {};
    };

    for (const auto& id : moduleIds) {
        auto result = visit(id);
        if (!result) {
            initializationOrder_.clear();
            return result;
        }
    }
    return {};
}

core::Result<void> ModuleManager::initializeAll() {
    auto result = resolveDependenciesAndSort();
    if (!result) return result;

    for (const auto& id : initializationOrder_) {
        IModule& module = *modules_.at(id);
        if (!canInitialize(module.state())) {
            return makeError(core::ErrorCode::StateError, "Module cannot be initialized from state " +
                std::string(stateName(module.state())) + ": " + id);
        }
        result = module.initialize();
        if (!result) {
            logger_.log(services::LogLevel::Error, "Runtime.ModuleManager", "Failed to initialize module: " + id);
            return result;
        }
        if (!canStart(module.state())) {
            return makeError(core::ErrorCode::StateError, "Module did not enter Initialized state: " + id);
        }
    }
    return {};
}

core::Result<void> ModuleManager::startAll() {
    std::vector<std::string> startedModules;
    const auto rollback = [&startedModules, this] {
        for (auto id = startedModules.rbegin(); id != startedModules.rend(); ++id) {
            IModule& module = *modules_.at(*id);
            if (module.state() == ModuleState::Started || module.state() == ModuleState::Running) {
                module.stop();
            }
        }
    };

    for (const auto& id : initializationOrder_) {
        IModule& module = *modules_.at(id);
        if (module.state() != ModuleState::Initialized) {
            rollback();
            return makeError(core::ErrorCode::StateError, "Module cannot be started from state " +
                std::string(stateName(module.state())) + ": " + id);
        }
        auto result = module.start();
        if (!result) {
            rollback();
            logger_.log(services::LogLevel::Error, "Runtime.ModuleManager", "Failed to start module: " + id);
            return result;
        }
        if (!canStop(module.state())) {
            rollback();
            return makeError(core::ErrorCode::StateError, "Module did not enter Started or Running state: " + id);
        }
        startedModules.push_back(id);
    }
    return {};
}

core::Result<void> ModuleManager::stopAll() {
    core::Result<void> firstError;
    bool hasError = false;
    for (auto id = initializationOrder_.rbegin(); id != initializationOrder_.rend(); ++id) {
        IModule& module = *modules_.at(*id);
        if (module.state() == ModuleState::Stopped || module.state() == ModuleState::Unloaded) {
            continue;
        }
        if (module.state() != ModuleState::Started && module.state() != ModuleState::Running) {
            if (!hasError) {
                firstError = makeError(core::ErrorCode::StateError, "Module cannot be stopped from state " +
                    std::string(stateName(module.state())) + ": " + *id);
                hasError = true;
            }
            continue;
        }
        auto result = module.stop();
        if (!result) {
            logger_.log(services::LogLevel::Error, "Runtime.ModuleManager", "Failed to stop module: " + *id);
            if (!hasError) {
                firstError = result;
                hasError = true;
            }
        } else if (module.state() != ModuleState::Stopped) {
            if (!hasError) {
                firstError = makeError(core::ErrorCode::StateError, "Module did not enter Stopped state: " + *id);
                hasError = true;
            }
        }
    }
    return hasError ? firstError : core::Result<void>{};
}

ModuleState ModuleManager::getModuleState(const std::string& moduleId) const {
    const auto found = modules_.find(moduleId);
    return found == modules_.end() ? ModuleState::Unloaded : found->second->state();
}

} // namespace framework::runtime
