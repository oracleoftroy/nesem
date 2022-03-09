#include "nes_cartridge.hpp"

#include <utility>

#include <util/logging.hpp>

#include "mappers/nes_mapper_000.hpp"
#include "mappers/nes_mapper_001.hpp"
#include "mappers/nes_mapper_002.hpp"
#include "mappers/nes_mapper_003.hpp"
#include "mappers/nes_mapper_066.hpp"
#include "mappers/nes_rom.hpp"

namespace nesem
{
	std::unique_ptr<NesCartridge> load_cartridge(mappers::NesRom rom) noexcept
	{
		LOG_INFO("mapper: {}", mapper(rom));

		switch (mirroring_mode(rom))
		{
			using enum mappers::ines_2::MirroringMode;
		case four_screen:
			LOG_INFO("mirroring: four-screen");
			break;
		case one_screen:
			LOG_INFO("mirroring: one-screen");
			break;
		case horizontal:
			LOG_INFO("mirroring: horizontal");
			break;
		case vertical:
			LOG_INFO("mirroring: vertical");
			break;
		}

		LOG_INFO("PRG-ROM banks: {}", prgrom_banks(rom));
		LOG_INFO("CHR-ROM banks: {}", chrrom_banks(rom));

		switch (mapper(rom))
		{
		default:
			LOG_WARN("ROM uses unsupported mapper: {}", mapper(rom));
			return {};

		case mappers::NesMapper000::ines_mapper:
			return std::make_unique<mappers::NesMapper000>(std::move(rom));

		case mappers::NesMapper001::ines_mapper:
			return std::make_unique<mappers::NesMapper001>(std::move(rom));

		case mappers::NesMapper002::ines_mapper:
			return std::make_unique<mappers::NesMapper002>(std::move(rom));

		case mappers::NesMapper003::ines_mapper:
			return std::make_unique<mappers::NesMapper003>(std::move(rom));

		case mappers::NesMapper066::ines_mapper:
			return std::make_unique<mappers::NesMapper066>(std::move(rom));
		}
	}
}
