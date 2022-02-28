#include "nes_cartridge.hpp"

#include <utility>

#include <util/logging.hpp>

#include "mapper/nes_mapper_000.hpp"
#include "mapper/nes_rom.hpp"

namespace nesem
{
	std::unique_ptr<NesCartridge> load_cartridge(const std::filesystem::path &filename) noexcept
	{
		auto rom = mapper::read_rom(filename);

		if (!rom)
			return {};

		switch (rom->mapper)
		{
		default:
			return {};

		case mapper::NesMapper000::ines_mapper:
			return std::make_unique<mapper::NesMapper000>(std::move(*rom));
		}
	}
}
