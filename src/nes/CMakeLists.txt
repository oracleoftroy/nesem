add_library(
	nesemlib
	"nes.cpp"
	"nes.hpp"
	"nes_bus.cpp"
	"nes_bus.hpp"
	"nes_cartridge.cpp"
	"nes_cartridge.hpp"
	"nes_clock.cpp"
	"nes_clock.hpp"
	"nes_cpu.cpp"
	"nes_cpu.hpp"
	"nes_cpu_ops.hpp"
)
target_link_libraries(nesemlib PRIVATE project_options)
target_link_libraries(nesemlib PUBLIC util)
target_include_directories(nesemlib PUBLIC ".")

add_executable(nesem WIN32 "nesem.manifest" "main.cpp")
target_link_libraries(nesem PRIVATE project_options)
target_link_libraries(nesem PRIVATE nesemlib)
target_link_libraries(nesem PRIVATE appui)
