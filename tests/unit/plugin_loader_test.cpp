#include "runtime/plugin_loader.h"
#include "runtime/module_manager.h"
#include "services/default_services.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <iostream>

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