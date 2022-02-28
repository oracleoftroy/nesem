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
	"nes_ppu.cpp"
	"nes_ppu.hpp"
	"mapper/nes_rom.cpp"
	"mapper/nes_rom.hpp"
	"mapper/nes_mapper_000.cpp"
	"mapper/nes_mapper_000.hpp"
	"mapper/nes_mapper_002.cpp"
	"mapper/nes_mapper_002.hpp"
	"mapper/nes_mapper_003.cpp"
	"mapper/nes_mapper_003.hpp"
)
target_link_libraries(nesemlib PRIVATE project_options)
target_link_libraries(nesemlib PUBLIC util)
target_include_directories(nesemlib PUBLIC ".")

add_executable(nesem WIN32 "nesem.manifest" "main.cpp" "text.cpp" "text.hpp")
target_link_libraries(nesem PRIVATE project_options)
target_link_libraries(nesem PRIVATE nesemlib)
target_link_libraries(nesem PRIVATE appui)
