find_package(Qt6 REQUIRED COMPONENTS Core Qml Quick)

qt_policy(SET QTP0001 NEW)
qt_standard_project_setup()

qt_add_executable(framework_qt6_example
    main.cpp
)

qt_add_qml_module(framework_qt6_example
    URI Framework.Qt6
    VERSION 1.0
    SOURCES
        adapters/runtime_bridge.cpp
        adapters/runtime_bridge.h
    QML_FILES
        Main.qml
)

target_include_directories(framework_qt6_example PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/adapters
)

target_link_libraries(framework_qt6_example PRIVATE
    framework_runtime
    framework_example_module
    Qt6::Quick
)

if(WIN32)
    find_program(QT_DEPLOY_TOOL
        NAMES windeployqt windeployqt.exe
        HINTS "${Qt6_DIR}/../../../bin"
    )

    if(QT_DEPLOY_TOOL)
        add_custom_command(TARGET framework_qt6_example POST_BUILD
            COMMAND "${QT_DEPLOY_TOOL}"
                --qmldir "${CMAKE_CURRENT_SOURCE_DIR}"
                "$<TARGET_FILE:framework_qt6_example>"
            VERBATIM
        )
    else()
        message(WARNING "windeployqt was not found; the Qt6 example may not run without Qt DLLs on PATH")
    endif()
endif()