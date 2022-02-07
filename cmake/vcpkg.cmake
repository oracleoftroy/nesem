include(FetchContent)

option(USE_VCPKG "Use vcpkg to provide dependencies" ON)

if(${USE_VCPKG})
	FetchContent_Declare(
		vcpkg
		GIT_REPOSITORY https://github.com/microsoft/vcpkg.git
		GIT_TAG master
	)

	# After the following call, the CMake targets defined by googletest and
	# Catch2 will be available to the rest of the build
	FetchContent_MakeAvailable(vcpkg)
	include(${vcpkg_SOURCE_DIR}/scripts/buildsystems/vcpkg.cmake)
endif()
