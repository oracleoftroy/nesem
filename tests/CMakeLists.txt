find_package(Catch2 CONFIG REQUIRED)

add_executable(nes-tests "run_nestest.cpp" "test_cpu_ops.cpp" "test_apu_channel.cpp")
target_link_libraries(nes-tests PRIVATE project_options)
target_link_libraries(nes-tests PRIVATE Catch2::Catch2WithMain)
target_link_libraries(nes-tests PRIVATE nesemlib)

if(EMSCRIPTEN)
	target_link_options(
		nes-tests
		PUBLIC
		-sASSERTIONS
		-sALLOW_MEMORY_GROWTH
		-sFORCE_FILESYSTEM
		--preload-file
		${PROJECT_SOURCE_DIR}/data@data
	)
endif()

include(Catch)
catch_discover_tests(nes-tests)
