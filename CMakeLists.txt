cmake_minimum_required(VERSION 3.22)
project(nes-project VERSION 0.0.1)

include(cmake/vcpkg.cmake)
include(cmake/PreventInSourceBuilds.cmake)
include(cmake/StandardProjectSettings.cmake)
include(cmake/CompilerWarnings.cmake)

add_library(project_options INTERFACE)
target_compile_features(project_options INTERFACE cxx_std_23)
set_project_warnings(project_options)

add_subdirectory(src)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	include(CTest)
endif()

if(BUILD_TESTING)
	add_subdirectory(tests)
endif()
