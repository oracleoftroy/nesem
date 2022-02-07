find_package(Catch2 CONFIG REQUIRED)

add_library(catch_main "main.cpp")
target_link_libraries(catch_main PUBLIC Catch2::Catch2)
target_link_libraries(catch_main PRIVATE project_options)

add_executable(nes-tests "test_cpu_ops.cpp")
target_link_libraries(nes-tests PRIVATE project_options)
target_link_libraries(nes-tests PRIVATE catch_main)
target_link_libraries(nes-tests PRIVATE nesemlib)

include(Catch)
catch_discover_tests(nes-tests)
