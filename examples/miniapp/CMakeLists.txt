add_executable(framework_miniapp
    main.cpp
)

target_link_libraries(framework_miniapp PRIVATE
    framework_runtime
    framework_example_module
)

if(BUILD_TESTING)
    add_test(NAME MiniAppSmoke COMMAND framework_miniapp)
endif()