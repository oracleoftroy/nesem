#include "nes_cartridge_loader.hpp"

#include <map>
#include <utility>

#include "nes_cartridge.hpp"

#include <util/logging.hpp>

// I don't think we ought to need to include these files as it should be enough for them to
// be included in the implementation file, but the types aren't being registered otherwise
// TODO: figure out why this isn't working like I expect and remove these includes
#include "mappers/nes_mapper_000.hpp"
#include "mappers/nes_mapper_001.hpp"
#include "mappers/nes_mapper_002.hpp"
#include "mappers/nes_mapper_003.hpp"
#include "mappers/nes_mapper_004.hpp"
#include "mappers/nes_mapper_005.hpp"
#include "mappers/nes_mapper_007.hpp"
#include "mappers/nes_mapper_009.hpp"
#include "mappers/nes_mapper_066.hpp"

namespace nesem::detail
{
	auto &cart_registry()
	{
		static std::map<int, MakeCartFn> registry;
		return registry;
	};

	bool register_cart(int ines_mapper, MakeCartFn &&fn)
	{
		auto [it, added] = cart_registry().emplace(ines_mapper, std::move(fn));
		return added;
	}
}

namespace nesem
{
	std::unique_ptr<NesCartridge> load_cartridge(const Nes &nes, mappers::NesRom rom) noexcept
	{
		if (rom.v2)
		{
			LOG_INFO("iNES 2 info");

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
				LOG_INFO("CHR ROM size: {0}K ({1:L})", rom.v2->chrrom->size / 1024, rom.v2->chrrom->size);

			if (rom.v2->chrram)
				LOG_INFO("CHR RAM size: {0}K ({1:L})", rom.v2->chrram->size / 1024, rom.v2->chrram->size);

			if (rom.v2->chrnvram)
				LOG_INFO("CHR NVRAM size: {0}K ({1:L})", rom.v2->chrnvram->size / 1024, rom.v2->chrnvram->size);
		}
		else
		{
			LOG_INFO("iNES 1 info");
			LOG_INFO("mapper: {}", rom_mapper(rom));
			LOG_INFO("PRG-ROM size: {0}K ({1:L})", size(rom.prg_rom) / 1024, size(rom.prg_rom));
			LOG_INFO("CHR-ROM size: {0}K ({1:L})", size(rom.chr_rom) / 1024, size(rom.chr_rom));
		}

		LOG_INFO("mirroring: {}", to_string(rom_mirroring_mode(rom)));
		LOG_INFO("has bus conflicts: {}", rom_has_bus_conflicts(rom));

		const auto &registry = detail::cart_registry();
		if (auto it = registry.find(rom_mapper(rom));
			it == end(registry))
		{
			LOG_WARN("ROM uses unsupported mapper: {}", rom_mapper(rom));
			return {};
		}
		else
			return std::invoke(it->second, nes, std::move(rom));
	}
}
