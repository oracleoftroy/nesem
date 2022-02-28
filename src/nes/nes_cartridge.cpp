#include "nes_cartridge.hpp"

#include <utility>

#include <util/logging.hpp>

#include "mapper/nes_mapper_000.hpp"
#include "mapper/nes_mapper_002.hpp"
#include "mapper/nes_rom.hpp"

namespace nesem
{
	std::unique_ptr<NesCartridge> load_cartridge(const std::filesystem::path &filename) noexcept
	{
		LOG_INFO("Loading ROM: {}", filename.string());
		auto rom = mapper::read_rom(filename);

		if (!rom)
		{
			LOG_WARN("Could not load ROM: {}", filename.string());
			return {};
		}

		LOG_INFO("iNES file version: {}", rom->version);
		LOG_INFO("mapper: {}", rom->mapper);
		LOG_INFO("mirroring: {}",
			rom->mirror_override                                     ? "dynamic"
				: rom->mirroring == mapper::NesMirroring::horizontal ? "horizontal"
																	 : "vertical");
		LOG_INFO("PRG-ROM banks: {}", rom->prg_rom_size);
		LOG_INFO("CHR-ROM banks: {}", rom->chr_rom_size);

		switch (rom->mapper)
		{
		default:
			LOG_WARN("ROM uses unsupported mapper: {}", rom->mapper);
			return {};

		case mapper::NesMapper000::ines_mapper:
			return std::make_unique<mapper::NesMapper000>(std::move(*rom));

		case mapper::NesMapper002::ines_mapper:
			return std::make_unique<mapper::NesMapper002>(std::move(*rom));
		}
	}
}
