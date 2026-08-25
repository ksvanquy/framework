#pragma once

#include "core/result.h"
#include "runtime/imodule.h"
#include "plugin_api.h"

#include <filesystem>
#include <memory>

namespace framework::runtime {

class LoadedPlugin {
public:
    using DestroyModule = void (*)(IModule*);

    LoadedPlugin() = default;
    LoadedPlugin(const PluginDescriptor& descriptor, IModule* module,
                 DestroyModule destroyModule, std::shared_ptr<void> library);
    ~LoadedPlugin();

    LoadedPlugin(const LoadedPlugin&) = delete;
    LoadedPlugin& operator=(const LoadedPlugin&) = delete;
    LoadedPlugin(LoadedPlugin&& other) noexcept;
    LoadedPlugin& operator=(LoadedPlugin&& other) noexcept;

    [[nodiscard]] const PluginDescriptor& descriptor() const;
    [[nodiscard]] IModule& module() const;

private:
    void reset() noexcept;

    const PluginDescriptor* descriptor_ = nullptr;
    IModule* module_ = nullptr;
    DestroyModule destroyModule_ = nullptr;
    std::shared_ptr<void> library_;
};

class PluginLoader {
public:
    static constexpr uint32_t CurrentApiVersion = FRAMEWORK_PLUGIN_API_VERSION;
    static constexpr uint32_t CurrentAbiVersion = FRAMEWORK_PLUGIN_ABI_VERSION;

    core::Result<LoadedPlugin> load(const std::filesystem::path& path) const;
};

} // namespace framework::runtime