find_package(cryptopp CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(mio CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(tinyxml2 CONFIG REQUIRED)

find_path(SCOTTT_DEBUGBREAK_INCLUDE_DIRS "debugbreak.h")

# provide a target for toml++
add_library(tomlplusplus INTERFACE)
add_library(tomlplusplus::tomlplusplus ALIAS tomlplusplus)
find_path(TOMLPLUSPLUS_INCLUDE_DIRS "toml++/toml.hpp" REQUIRED)
target_include_directories(tomlplusplus INTERFACE ${TOMLPLUSPLUS_INCLUDE_DIRS})

# Fixup libfmt targets
if(NOT MSVC)
	get_target_property(FMT_OPTS fmt::fmt INTERFACE_COMPILE_OPTIONS)
	get_target_property(FMTHO_OPTS fmt::fmt-header-only INTERFACE_COMPILE_OPTIONS)

	# For some reason, newer versions of FMT are adding the /utf-8 option even when building with clang... not sure if vcpkg issue or upstream
	# For now, remove the option if it exists
	string(REPLACE "\$<\$<COMPILE_LANGUAGE:CXX>:/utf-8>" "" FMT_OPTS ${FMT_OPTS})
	string(REPLACE "\$<\$<COMPILE_LANGUAGE:CXX>:/utf-8>" "" FMTHO_OPTS ${FMTHO_OPTS})

	set_target_properties(fmt::fmt PROPERTIES INTERFACE_COMPILE_OPTIONS "${FMT_OPTS}")
	set_target_properties(fmt::fmt-header-only PROPERTIES INTERFACE_COMPILE_OPTIONS "${FMTHO_OPTS}")
endif()

add_subdirectory(util)
add_subdirectory(app)
add_subdirectory(nes)
add_subdirectory(main)

option(BUILD_TOOLS "build additional tools" OFF)

if(${BUILD_TOOLS})
	add_subdirectory(tools)
endif()
