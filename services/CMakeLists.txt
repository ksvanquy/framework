add_library(framework_services STATIC
    src/default_services.cpp
    include/services/default_services.h
    include/services/ilogger.h
    include/services/ievent_bus.h
    include/services/iconfig.h
    include/services/icommand_bus.h
    include/services/ischeduler.h
    include/services/istorage.h
    include/services/idiagnostics.h
)

target_include_directories(framework_services
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(framework_services PUBLIC framework_core)