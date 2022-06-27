#include "nes_cartridge.hpp"

#include <utility>

#include "mappers/nes_mapper_000.hpp"
#include "mappers/nes_mapper_001.hpp"
#include "mappers/nes_mapper_002.hpp"
#include "mappers/nes_mapper_003.hpp"
#include "mappers/nes_mapper_004.hpp"
#include "mappers/nes_mapper_066.hpp"
#include "mappers/nes_rom.hpp"

#include <util/logging.hpp>

namespace nesem
{
	NesCartridge::NesCartridge(mappers::NesRom &&rom) noexcept
		: rom(std::move(rom))
	{
	}

	const mappers::NesRom &NesCartridge::get_rom() noexcept
	{
		return rom;
	}

	bool NesCartridge::irq() noexcept
	{
		return irq_signaled;
	}

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

		LOG_INFO("PRG-ROM size: {0:X} ({0:L})", size(rom.prg_rom));
		LOG_INFO("CHR-ROM size: {0:X} ({0:L})", size(rom.chr_rom));

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

		case mappers::NesMapper004::ines_mapper:
			return std::make_unique<mappers::NesMapper004>(std::move(rom));

		case mappers::NesMapper066::ines_mapper:
			return std::make_unique<mappers::NesMapper066>(std::move(rom));
		}
	}
}
