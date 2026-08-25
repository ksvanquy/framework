add_library(framework_runtime STATIC
    src/module_manager.cpp
    src/plugin_loader.cpp
    src/runtime.cpp
    include/runtime/imodule.h
    include/runtime/module_manager.h
    include/runtime/plugin_loader.h
    include/runtime/runtime.h
    include/runtime/application.h
    include/runtime/context.h
)

target_include_directories(framework_runtime
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../plugins/plugin_sdk>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(framework_runtime PUBLIC framework_services)

if(UNIX AND NOT APPLE)
    target_link_libraries(framework_runtime PRIVATE ${CMAKE_DL_LIBS})
endif()