add_library(
	nesemlib
	"nes.cpp"
	"nes.hpp"
	"nes_apu.cpp"
	"nes_apu.hpp"
	"nes_bus.cpp"
	"nes_bus.hpp"
	"nes_cartridge.cpp"
	"nes_cartridge.hpp"
	"nes_clock.cpp"
	"nes_clock.hpp"
	"nes_cpu.cpp"
	"nes_cpu.hpp"
	"nes_cpu_ops.hpp"
	"nes_input_device.hpp"
	"nes_nvram.cpp"
	"nes_nvram.hpp"
	"nes_ppu.cpp"
	"nes_ppu.hpp"
	"nes_rom.cpp"
	"nes_rom.hpp"
	"nes_rom_loader.cpp"
	"nes_rom_loader.hpp"
	"nes_sha1.cpp"
	"nes_sha1.hpp"
	"mappers/nes_mapper_000.cpp"
	"mappers/nes_mapper_000.hpp"
	"mappers/nes_mapper_001.cpp"
	"mappers/nes_mapper_001.hpp"
	"mappers/nes_mapper_002.cpp"
	"mappers/nes_mapper_002.hpp"
	"mappers/nes_mapper_003.cpp"
	"mappers/nes_mapper_003.hpp"
	"mappers/nes_mapper_004.cpp"
	"mappers/nes_mapper_004.hpp"
	"mappers/nes_mapper_007.cpp"
	"mappers/nes_mapper_007.hpp"
	"mappers/nes_mapper_009.cpp"
	"mappers/nes_mapper_009.hpp"
	"mappers/nes_mapper_066.cpp"
	"mappers/nes_mapper_066.hpp"
)

find_package(tinyxml2 CONFIG REQUIRED)
find_package(cryptopp CONFIG REQUIRED)
find_package(mio CONFIG REQUIRED)

target_link_libraries(nesemlib PRIVATE project_options)
target_link_libraries(nesemlib PUBLIC util)
target_link_libraries(nesemlib PRIVATE tinyxml2::tinyxml2)
target_link_libraries(nesemlib PRIVATE cryptopp-static)
target_link_libraries(nesemlib PRIVATE mio::mio)
target_include_directories(nesemlib PUBLIC ".")
