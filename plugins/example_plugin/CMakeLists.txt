add_library(framework_example_plugin SHARED
    example_plugin.cpp
)

add_library(framework_invalid_api_plugin SHARED invalid_plugin.cpp)
add_library(framework_invalid_abi_plugin SHARED invalid_plugin.cpp)

target_include_directories(framework_example_plugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../plugin_sdk
    ${CMAKE_SOURCE_DIR}/runtime/include
)

target_link_libraries(framework_example_plugin PRIVATE framework_runtime)
target_link_libraries(framework_invalid_api_plugin PRIVATE framework_runtime)
target_link_libraries(framework_invalid_abi_plugin PRIVATE framework_runtime)
target_compile_definitions(framework_invalid_api_plugin PRIVATE INVALID_PLUGIN_API_VERSION=999)
target_compile_definitions(framework_invalid_abi_plugin PRIVATE INVALID_PLUGIN_ABI_VERSION=999)

if(BUILD_PLUGINS)
    install(TARGETS framework_example_plugin
        RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/framework/plugins
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/framework/plugins
    )
endif()