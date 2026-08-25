# CMake generated Testfile for 
# Source directory: D:/DEV/framework/examples/miniapp
# Build directory: D:/DEV/framework/build/examples/miniapp
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test("MiniAppSmoke" "D:/DEV/framework/build/examples/miniapp/Debug/framework_miniapp.exe")
  set_tests_properties("MiniAppSmoke" PROPERTIES  _BACKTRACE_TRIPLES "D:/DEV/framework/examples/miniapp/CMakeLists.txt;11;add_test;D:/DEV/framework/examples/miniapp/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test("MiniAppSmoke" "D:/DEV/framework/build/examples/miniapp/Release/framework_miniapp.exe")
  set_tests_properties("MiniAppSmoke" PROPERTIES  _BACKTRACE_TRIPLES "D:/DEV/framework/examples/miniapp/CMakeLists.txt;11;add_test;D:/DEV/framework/examples/miniapp/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test("MiniAppSmoke" "D:/DEV/framework/build/examples/miniapp/MinSizeRel/framework_miniapp.exe")
  set_tests_properties("MiniAppSmoke" PROPERTIES  _BACKTRACE_TRIPLES "D:/DEV/framework/examples/miniapp/CMakeLists.txt;11;add_test;D:/DEV/framework/examples/miniapp/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test("MiniAppSmoke" "D:/DEV/framework/build/examples/miniapp/RelWithDebInfo/framework_miniapp.exe")
  set_tests_properties("MiniAppSmoke" PROPERTIES  _BACKTRACE_TRIPLES "D:/DEV/framework/examples/miniapp/CMakeLists.txt;11;add_test;D:/DEV/framework/examples/miniapp/CMakeLists.txt;0;")
else()
  add_test("MiniAppSmoke" NOT_AVAILABLE)
endif()
