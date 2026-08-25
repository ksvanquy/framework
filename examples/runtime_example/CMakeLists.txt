add_executable(framework_runtime_example
    main.cpp
)

target_link_libraries(framework_runtime_example PRIVATE
    framework_runtime
    framework_example_module
)