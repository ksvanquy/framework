#include "runtime/plugin_loader.h"
#include "runtime/module_manager.h"
#include "services/default_services.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

TEST(PluginLoaderTest, LoadsValidPluginAndOwnsModuleLifetime) {
    framework::runtime::PluginLoader loader;
    auto result = loader.load(FRAMEWORK_EXAMPLE_PLUGIN_PATH);

    ASSERT_TRUE(result);
    auto plugin = std::move(result.value());
    EXPECT_STREQ(plugin.descriptor().id, "example.plugin");
    EXPECT_EQ(plugin.descriptor().apiVersion,
              framework::runtime::PluginLoader::CurrentApiVersion);
    EXPECT_EQ(plugin.descriptor().abiVersion,
              framework::runtime::PluginLoader::CurrentAbiVersion);
    EXPECT_EQ(plugin.descriptor().dependencyCount, 0U);
    EXPECT_EQ(plugin.descriptor().dependencies, nullptr);
    EXPECT_EQ(plugin.module().state(), framework::runtime::ModuleState::Discovered);
    EXPECT_TRUE(plugin.module().initialize());
    EXPECT_TRUE(plugin.module().start());
    EXPECT_EQ(plugin.module().state(), framework::runtime::ModuleState::Running);
    EXPECT_TRUE(plugin.module().stop());
    EXPECT_EQ(plugin.module().state(), framework::runtime::ModuleState::Stopped);
}

TEST(PluginLoaderTest, RejectsMissingPlugin) {
    framework::runtime::PluginLoader loader;

    const auto result = loader.load(std::filesystem::path("missing-plugin.dll"));

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), framework::core::ErrorCode::PluginLoadFailed);
}

TEST(PluginLoaderTest, RejectsIncompatibleApiVersion) {
    framework::runtime::PluginLoader loader;

    const auto result = loader.load(FRAMEWORK_INVALID_API_PLUGIN_PATH);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), framework::core::ErrorCode::PluginLoadFailed);
}

TEST(PluginLoaderTest, RejectsIncompatibleAbiVersion) {
    framework::runtime::PluginLoader loader;

    const auto result = loader.load(FRAMEWORK_INVALID_ABI_PLUGIN_PATH);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), framework::core::ErrorCode::PluginLoadFailed);
}

TEST(PluginLoaderTest, RejectsMissingRequiredExport) {
    framework::runtime::PluginLoader loader;
    const std::vector<std::pair<const char*, const char*>> fixtures{
        {FRAMEWORK_MISSING_DESCRIPTOR_PLUGIN_PATH, "get_plugin_descriptor"},
        {FRAMEWORK_MISSING_CREATE_PLUGIN_PATH, "create_plugin_module"},
        {FRAMEWORK_MISSING_DESTROY_PLUGIN_PATH, "destroy_plugin_module"}};

    for (const auto& [path, missingSymbol] : fixtures) {
        const auto result = loader.load(path);

        ASSERT_FALSE(result) << "Expected missing export: " << missingSymbol;
        EXPECT_EQ(result.error().code(), framework::core::ErrorCode::PluginLoadFailed);
        EXPECT_NE(result.error().message().find(missingSymbol), std::string::npos);
    }
}

TEST(PluginLoaderTest, RejectsCreateReturningNull) {
    framework::runtime::PluginLoader loader;

    const auto result = loader.load(FRAMEWORK_NULL_MODULE_PLUGIN_PATH);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), framework::core::ErrorCode::PluginLoadFailed);
    EXPECT_NE(result.error().message().find("Plugin module creation failed"), std::string::npos);
}

TEST(PluginLoaderTest, RejectsMalformedDescriptor) {
    framework::runtime::PluginLoader loader;
    const std::vector<const char*> fixtures{
        FRAMEWORK_MALFORMED_NULL_DESCRIPTOR_PLUGIN_PATH,
        FRAMEWORK_MALFORMED_EMPTY_ID_PLUGIN_PATH,
        FRAMEWORK_MALFORMED_EMPTY_NAME_PLUGIN_PATH,
        FRAMEWORK_MALFORMED_NULL_DEPENDENCIES_PLUGIN_PATH,
        FRAMEWORK_MALFORMED_DUPLICATE_DEPENDENCIES_PLUGIN_PATH};

    for (const auto* path : fixtures) {
        const auto result = loader.load(path);

        ASSERT_FALSE(result) << "Expected malformed descriptor: " << path;
        EXPECT_EQ(result.error().code(), framework::core::ErrorCode::InvalidArgument);
        EXPECT_NE(result.error().message().find("invalid"), std::string::npos);
    }
}

TEST(PluginLoaderTest, RegistersLoadedPluginWithModuleManager) {
    framework::services::ConsoleLogger logger(std::clog);
    framework::runtime::PluginLoader loader;
    auto plugin = loader.load(FRAMEWORK_EXAMPLE_PLUGIN_PATH);
    ASSERT_TRUE(plugin);
    framework::runtime::ModuleManager manager(logger);

    ASSERT_TRUE(manager.registerPlugin(std::move(plugin.value())));
    EXPECT_EQ(manager.getModuleState("example.plugin"),
              framework::runtime::ModuleState::Discovered);
    ASSERT_TRUE(manager.initializeAll());
    ASSERT_TRUE(manager.startAll());
    EXPECT_EQ(manager.getModuleState("example.plugin"),
              framework::runtime::ModuleState::Running);
    ASSERT_TRUE(manager.stopAll());
    EXPECT_EQ(manager.getModuleState("example.plugin"),
              framework::runtime::ModuleState::Stopped);
}

TEST(PluginLoaderTest, UnloadsPluginAfterStoppingItsModule) {
    framework::services::ConsoleLogger logger(std::clog);
    framework::runtime::PluginLoader loader;
    auto plugin = loader.load(FRAMEWORK_EXAMPLE_PLUGIN_PATH);
    ASSERT_TRUE(plugin);
    framework::runtime::ModuleManager manager(logger);
    ASSERT_TRUE(manager.registerPlugin(std::move(plugin.value())));
    ASSERT_TRUE(manager.initializeAll());
    ASSERT_TRUE(manager.startAll());

    ASSERT_TRUE(manager.unloadPlugin("example.plugin"));
    EXPECT_EQ(manager.getModuleState("example.plugin"),
              framework::runtime::ModuleState::Unloaded);
    EXPECT_FALSE(manager.unloadPlugin("example.plugin"));
}

} // namespace