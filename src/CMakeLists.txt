option(BUILD_TOOLS "build additional tools" OFF)

add_subdirectory(util)
add_subdirectory(app)
add_subdirectory(nes)
add_subdirectory(main)

if(${BUILD_TOOLS})
	add_subdirectory(tools)
endif()
