include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(framework_core_tests
    unit/error_test.cpp
    unit/result_test.cpp
    unit/runtime_test.cpp
    unit/plugin_loader_test.cpp
    integration/core_headers_test.cpp
    integration/lifetime_test.cpp
    integration/application_test.cpp
    stress/result_stress_test.cpp
    system/core_smoke_test.cpp
)

target_compile_features(framework_core_tests PRIVATE cxx_std_20)
target_include_directories(framework_core_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(framework_core_tests PRIVATE framework_core framework_runtime framework_example_module GTest::gtest_main)
target_compile_definitions(framework_core_tests PRIVATE
    FRAMEWORK_EXAMPLE_PLUGIN_PATH="$<TARGET_FILE:framework_example_plugin>"
    FRAMEWORK_INVALID_API_PLUGIN_PATH="$<TARGET_FILE:framework_invalid_api_plugin>"
    FRAMEWORK_INVALID_ABI_PLUGIN_PATH="$<TARGET_FILE:framework_invalid_abi_plugin>")

include(GoogleTest)
gtest_discover_tests(framework_core_tests)
