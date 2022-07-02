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
	NesCartridge::NesCartridge(const Nes &nes, mappers::NesRom &&rom) noexcept
		: nes(&nes), rom(std::move(rom))
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

	std::unique_ptr<NesCartridge> load_cartridge(const Nes &nes, mappers::NesRom rom) noexcept
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

		LOG_INFO("PRG-ROM size: {0}K ({1:L})", size(rom.prg_rom) / 1024, size(rom.prg_rom));
		LOG_INFO("CHR-ROM size: {0}K ({1:L})", size(rom.chr_rom) / 1024, size(rom.chr_rom));

		if (rom.v2)
		{
			LOG_INFO("iNES2 info");

			LOG_INFO("Console region: {0}, type: {1}", rom.v2->console.region, rom.v2->console.type);
			LOG_INFO("Expansion device: {0}", expansion_device_name(rom.v2->expansion));
			LOG_INFO("mapper: {0}, submapper: {1}", rom.v2->pcb.mapper, rom.v2->pcb.submapper);
			LOG_INFO("has battery: {0}", rom.v2->pcb.battery);

			LOG_INFO("PRG ROM size: {0}K ({1:L})", rom.v2->prgrom.size / 1024, rom.v2->prgrom.size);

			if (rom.v2->prgram)
				LOG_INFO("PRG RAM size: {0}K ({1:L})", rom.v2->prgram->size / 1024, rom.v2->prgram->size);

			if (rom.v2->prgnvram)
				LOG_INFO("PRG NVRAM size: {0}K ({1:L})", rom.v2->prgnvram->size / 1024, rom.v2->prgnvram->size);

			if (rom.v2->chrrom)
				LOG_INFO("CHR-ROM size: {0}K ({1:L})", rom.v2->chrrom->size / 1024, rom.v2->chrrom->size);

			if (rom.v2->chrram)
				LOG_INFO("CHR-RAM size: {0}K ({1:L})", rom.v2->chrram->size / 1024, rom.v2->chrram->size);

			if (rom.v2->chrnvram)
				LOG_INFO("CHR NVRAM size: {0}K ({1:L})", rom.v2->chrnvram->size / 1024, rom.v2->chrnvram->size);
		}

		switch (mapper(rom))
		{
		default:
			LOG_WARN("ROM uses unsupported mapper: {}", mapper(rom));
			return {};

		case mappers::NesMapper000::ines_mapper:
			return std::make_unique<mappers::NesMapper000>(nes, std::move(rom));

		case mappers::NesMapper001::ines_mapper:
			return std::make_unique<mappers::NesMapper001>(nes, std::move(rom));

		case mappers::NesMapper002::ines_mapper:
			return std::make_unique<mappers::NesMapper002>(nes, std::move(rom));

		case mappers::NesMapper003::ines_mapper:
			return std::make_unique<mappers::NesMapper003>(nes, std::move(rom));

		case mappers::NesMapper004::ines_mapper:
			return std::make_unique<mappers::NesMapper004>(nes, std::move(rom));

		case mappers::NesMapper066::ines_mapper:
			return std::make_unique<mappers::NesMapper066>(nes, std::move(rom));
		}
	}
}
