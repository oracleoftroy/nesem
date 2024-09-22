# For some reason, newer versions of FMT on Windows are adding the /utf-8 option even when building with clang... not sure if vcpkg issue or upstream
# This removes the option if it exists and the compiler isn't MSVC
# TODO: clang-cl? IIRC, it is the default, but need to verify...
function(fixup_libfmt_targets fmt_target_name)
	target_compile_options(${fmt_target_name} INTERFACE ${PROJECT_WARNINGS})

	# Fixup libfmt targets
	if(NOT MSVC)
		get_target_property(FMT_OPTS ${fmt_target_name} INTERFACE_COMPILE_OPTIONS)

		if(FMT_OPTS)
			string(REPLACE "\$<\$<COMPILE_LANGUAGE:CXX>:/utf-8>" "" FMT_OPTS ${FMT_OPTS})
			set_target_properties(${fmt_target_name} PROPERTIES INTERFACE_COMPILE_OPTIONS "${FMT_OPTS}")
		endif()
	endif()
endfunction()

find_package(cryptopp CONFIG REQUIRED)
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

find_package(fmt CONFIG REQUIRED)
fixup_libfmt_targets(fmt::fmt)
fixup_libfmt_targets(fmt::fmt-header-only)

add_subdirectory(util)
add_subdirectory(app)
add_subdirectory(nes)
add_subdirectory(main)

option(BUILD_TOOLS "build additional tools" OFF)

if(${BUILD_TOOLS})
	add_subdirectory(tools)
endif()
