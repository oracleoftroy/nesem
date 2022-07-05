cmake_minimum_required(VERSION 3.22)
project(nesem VERSION 0.0.1)

include(cmake/vcpkg.cmake)
include(cmake/PreventInSourceBuilds.cmake)
include(cmake/StandardProjectSettings.cmake)
include(cmake/CompilerWarnings.cmake)

add_library(project_options INTERFACE)
target_compile_features(project_options INTERFACE cxx_std_23)
set_project_warnings(project_options)
target_compile_definitions(project_options INTERFACE "$<$<CONFIG:Debug,RelWithDebInfo>:DEBUG>")

add_subdirectory(src)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	include(CTest)
endif()

if(BUILD_TESTING)
	add_subdirectory(tests)
endif()

if(WIN32)
	set(CPACK_GENERATOR "ZIP")
	set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
	set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION nesem)

	install(TARGETS nesem RUNTIME DESTINATION nesem)
	install(FILES $<TARGET_RUNTIME_DLLS:nesem> DESTINATION nesem)
	install(FILES data/nes20db.xml DESTINATION nesem/data)

else()
	install(TARGETS nesem)
endif()

include(CPack)
include(InstallRequiredSystemLibraries)
