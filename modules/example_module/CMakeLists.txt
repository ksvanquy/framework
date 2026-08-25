add_library(framework_example_module STATIC
    src/example_module.cpp
    include/example_module/example_module.h
)

target_include_directories(framework_example_module
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(framework_example_module PUBLIC framework_runtime)