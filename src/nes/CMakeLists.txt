add_library(nesemlib)

target_include_directories(nesemlib PUBLIC "include")
target_sources(
	nesemlib
	PUBLIC "include/nes_addr.hpp"
			"include/nes_apu.hpp"
			"include/nes_bus.hpp"
			"include/nes_cartridge.hpp"
			"include/nes_clock.hpp"
			"include/nes_cpu.hpp"
			"include/nes_input_device.hpp"
			"include/nes_nvram.hpp"
			"include/nes_ppu.hpp"
			"include/nes_rom_loader.hpp"
			"include/nes_rom.hpp"
			"include/nes_types.hpp"
			"include/nes.hpp"
	PRIVATE "src/nes_apu.cpp"
			"src/nes_bus.cpp"
			"src/nes_cartridge_loader.hpp"
			"src/nes_cartridge_loader.cpp"
			"src/nes_cartridge.cpp"
			"src/nes_clock.cpp"
			"src/nes_cpu_ops.hpp"
			"src/nes_cpu.cpp"
			"src/nes_nvram.cpp"
			"src/nes_ppu_register_bits.hpp"
			"src/nes_ppu.cpp"
			"src/nes_rom_loader.cpp"
			"src/nes_rom.cpp"
			"src/nes_sha1.cpp"
			"src/nes_sha1.hpp"
			"src/nes.cpp"
			"src/mappers/nes_mapper_000.cpp"
			"src/mappers/nes_mapper_000.hpp"
			"src/mappers/nes_mapper_001.cpp"
			"src/mappers/nes_mapper_001.hpp"
			"src/mappers/nes_mapper_002.cpp"
			"src/mappers/nes_mapper_002.hpp"
			"src/mappers/nes_mapper_003.cpp"
			"src/mappers/nes_mapper_003.hpp"
			"src/mappers/nes_mapper_004.cpp"
			"src/mappers/nes_mapper_004.hpp"
			"src/mappers/nes_mapper_005.cpp"
			"src/mappers/nes_mapper_005.hpp"
			"src/mappers/nes_mapper_007.cpp"
			"src/mappers/nes_mapper_007.hpp"
			"src/mappers/nes_mapper_009.cpp"
			"src/mappers/nes_mapper_009.hpp"
			"src/mappers/nes_mapper_066.cpp"
			"src/mappers/nes_mapper_066.hpp"
)

find_package(tinyxml2 CONFIG REQUIRED)
find_package(cryptopp CONFIG REQUIRED)
find_package(mio CONFIG REQUIRED)

target_link_libraries(nesemlib PRIVATE project_options)
target_link_libraries(nesemlib PUBLIC util)
target_link_libraries(nesemlib PRIVATE tinyxml2::tinyxml2)
target_link_libraries(nesemlib PRIVATE cryptopp::cryptopp)
target_link_libraries(nesemlib PRIVATE mio::mio)
