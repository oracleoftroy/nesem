include(FetchContent)

option(USE_VCPKG "Use vcpkg to provide dependencies" ON)

if(${USE_VCPKG})
	include(ext/vcpkg/scripts/buildsystems/vcpkg.cmake)
endif()

