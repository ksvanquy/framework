cmake_minimum_required(VERSION 3.20)
project(CppApplicationFramework VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_EXAMPLES "Build framework example applications" ON)
option(BUILD_PLUGINS "Build example plugins" ON)
option(BUILD_QT6 "Build the optional Qt6/QML adapter and example" OFF)
option(ENABLE_WARNINGS "Enable strict compiler warnings" ON)
option(ENABLE_CLANG_TIDY "Run clang-tidy during compilation" OFF)
option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

if(ENABLE_CLANG_TIDY)
	find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy REQUIRED)
	set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXECUTABLE}")
endif()

if(ENABLE_SANITIZERS AND MSVC)
	message(FATAL_ERROR "ENABLE_SANITIZERS requires GCC or Clang")
endif()

add_subdirectory(core)
add_subdirectory(services)
add_subdirectory(runtime)
add_subdirectory(modules/example_module)
include(CTest)

if(BUILD_QT6)
	add_subdirectory(ui/qt6)
endif()

if(BUILD_PLUGINS OR BUILD_TESTING)
	add_subdirectory(plugins/example_plugin)
endif()
if(BUILD_EXAMPLES)
	add_subdirectory(examples/runtime_example)
	add_subdirectory(examples/miniapp)
endif()

if(BUILD_TESTING)
	add_subdirectory(tests)
endif()

set(FRAMEWORK_COMPILE_TARGETS framework_services framework_runtime framework_example_module)
if(BUILD_PLUGINS OR BUILD_TESTING)
	list(APPEND FRAMEWORK_COMPILE_TARGETS framework_example_plugin
		framework_invalid_api_plugin framework_invalid_abi_plugin)
endif()
if(BUILD_EXAMPLES)
	list(APPEND FRAMEWORK_COMPILE_TARGETS framework_runtime_example)
	list(APPEND FRAMEWORK_COMPILE_TARGETS framework_miniapp)
endif()
if(BUILD_TESTING)
	list(APPEND FRAMEWORK_COMPILE_TARGETS framework_core_tests)
endif()

foreach(target IN LISTS FRAMEWORK_COMPILE_TARGETS)
	if(TARGET ${target})
		if(ENABLE_WARNINGS)
			if(MSVC)
				target_compile_options(${target} PRIVATE /W4 /WX /permissive-)
			else()
				target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
			endif()
		endif()
		if(ENABLE_SANITIZERS)
			target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
			target_link_options(${target} PRIVATE -fsanitize=address,undefined)
		endif()
	endif()
endforeach()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS framework_core framework_services framework_runtime framework_example_module
	EXPORT FrameworkTargets
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY core/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY services/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY runtime/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES plugins/plugin_sdk/plugin_api.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY modules/example_module/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT FrameworkTargets
	FILE FrameworkTargets.cmake
	NAMESPACE Framework::
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Framework
)
configure_package_config_file(
	cmake/FrameworkConfig.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/FrameworkConfig.cmake
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Framework
)
write_basic_package_version_file(
	${CMAKE_CURRENT_BINARY_DIR}/FrameworkConfigVersion.cmake
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY SameMajorVersion
)
install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/FrameworkConfig.cmake
	${CMAKE_CURRENT_BINARY_DIR}/FrameworkConfigVersion.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Framework
)
