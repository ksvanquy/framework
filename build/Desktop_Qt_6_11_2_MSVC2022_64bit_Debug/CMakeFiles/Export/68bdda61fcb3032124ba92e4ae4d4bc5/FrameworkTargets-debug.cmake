#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Framework::framework_services" for configuration "Debug"
set_property(TARGET Framework::framework_services APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(Framework::framework_services PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/framework_services.lib"
  )

list(APPEND _cmake_import_check_targets Framework::framework_services )
list(APPEND _cmake_import_check_files_for_Framework::framework_services "${_IMPORT_PREFIX}/lib/framework_services.lib" )

# Import target "Framework::framework_runtime" for configuration "Debug"
set_property(TARGET Framework::framework_runtime APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(Framework::framework_runtime PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/framework_runtime.lib"
  )

list(APPEND _cmake_import_check_targets Framework::framework_runtime )
list(APPEND _cmake_import_check_files_for_Framework::framework_runtime "${_IMPORT_PREFIX}/lib/framework_runtime.lib" )

# Import target "Framework::framework_example_module" for configuration "Debug"
set_property(TARGET Framework::framework_example_module APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(Framework::framework_example_module PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/framework_example_module.lib"
  )

list(APPEND _cmake_import_check_targets Framework::framework_example_module )
list(APPEND _cmake_import_check_files_for_Framework::framework_example_module "${_IMPORT_PREFIX}/lib/framework_example_module.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
