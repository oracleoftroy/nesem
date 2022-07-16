#include "nes_cartridge.hpp"

#include <utility>

#include "mappers/nes_mapper_000.hpp"
#include "mappers/nes_mapper_001.hpp"
#include "mappers/nes_mapper_002.hpp"
#include "mappers/nes_mapper_003.hpp"
#include "mappers/nes_mapper_004.hpp"
#include "mappers/nes_mapper_007.hpp"
#include "mappers/nes_mapper_009.hpp"
#include "mappers/nes_mapper_066.hpp"
#include "nes.hpp"

#include <util/logging.hpp>

namespace nesem
{
	NesCartridge::NesCartridge(const Nes &nes, mappers::NesRom &&rom_data) noexcept
		: nes(&nes), nes_rom(std::move(rom_data))
	{
		if (has_chrram(rom()))
		{
			if (!rom().chr_rom.empty())
				LOG_WARN("CHR-ROM not empty, but we assume CHR-ROM and CHR-RAM are mutually exclusive!");

			chr_ram.resize(chrram_size(rom()));
		}

		if (rom().v2)
		{
			if (rom().v2->prgram)
				prg_ram.resize(rom().v2->prgram->size);

			if (rom().v2->prgnvram)
				prg_nvram = nes.open_prgnvram(rom().v2->rom.sha1, rom().v2->prgnvram->size);
		}
		else if (auto size = rom().v1.prg_ram_size * bank_8k;
				 size > 0)
		{
			// nesdev wiki indicates ram size in iNES 1 isn't reliable, but if its all we have to go by...
			if (rom().v1.has_battery)
				prg_nvram = nes.open_prgnvram(rom().sha1, size);
			else
				prg_ram.resize(size);
		}

		emulate_bus_conflicts = has_bus_conflicts(nes_rom);
	}

	mappers::MirroringMode NesCartridge::mirroring() const noexcept
	{
		// default implementation returns whatever the ROM tells us
		return mirroring_mode(nes_rom);
	}

	U8 NesCartridge::cpu_peek(U16 addr) const noexcept
	{
		return on_cpu_peek(addr);
	}

	U8 NesCartridge::cpu_read(U16 addr) noexcept
	{
		return on_cpu_read(addr);
	}

	void NesCartridge::cpu_write(U16 addr, U8 value) noexcept
	{
		if (emulate_bus_conflicts)
			value &= cpu_peek(addr);

		on_cpu_write(addr, value);
	}

	std::optional<U8> NesCartridge::ppu_peek(U16 &addr) const noexcept
	{
		if (addr >= 0x4000) [[unlikely]]
			LOG_ERROR("address out of range? PPU should properly mirror addresses, but we got ${:04X}", addr);

		return on_ppu_peek(addr);
	}

	std::optional<U8> NesCartridge::ppu_read(U16 &addr) noexcept
	{
		if (addr >= 0x4000) [[unlikely]]
			LOG_ERROR("address out of range? PPU should properly mirror addresses, but we got ${:04X}", addr);

		return on_ppu_read(addr);
	}

	bool NesCartridge::ppu_write(U16 &addr, U8 value) noexcept
	{
		if (addr >= 0x4000) [[unlikely]]
			LOG_ERROR("address out of range? PPU should properly mirror addresses, but we got ${:04X}", addr);

		return on_ppu_write(addr, value);
	}

	U8 NesCartridge::on_cpu_read(U16 addr) noexcept
	{
		return on_cpu_peek(addr);
	}

	std::optional<U8> NesCartridge::on_ppu_read(U16 &addr) noexcept
	{
		return on_ppu_peek(addr);
	}

	const mappers::NesRom &NesCartridge::rom() const noexcept
	{
		return nes_rom;
	}

	bool NesCartridge::irq() noexcept
	{
		return irq_signaled;
	}

	size_t NesCartridge::chr_size() const noexcept
	{
		if (has_chrram(nes_rom))
			return size(chr_ram);

		return size(nes_rom.chr_rom);
	}

	U8 NesCartridge::chr_read(size_t addr) const noexcept
	{
		if (has_chrram(nes_rom))
			return chr_ram[addr];

		return nes_rom.chr_rom[addr];
	}

	bool NesCartridge::chr_write(size_t addr, U8 value) noexcept
	{
		if (has_chrram(nes_rom))
			chr_ram[addr] = value;
		else
			LOG_ERROR("Write to CHR-ROM not allowed");

		return true;
	}

	size_t NesCartridge::cpu_ram_size() const noexcept
	{
		if (prgram_size() > 0 && prgnvram_size() > 0) [[unlikely]]
			LOG_ERROR("Not expecting cart to use both prgram and prgnvram");

		return prgnvram_size() + prgram_size();
	}

	U8 NesCartridge::cpu_ram_read(size_t addr) const noexcept
	{
		if (prgram_size() > 0 && prgnvram_size() > 0) [[unlikely]]
			LOG_ERROR("Not expecting cart to use both prgram and prgnvram");

		if (auto size = prgram_size();
			size > 0)
			return prgram_read(addr);

		if (auto size = prgnvram_size();
			size > 0)
			return prgnvram_read(addr);

		return open_bus_read();
	}

	bool NesCartridge::cpu_ram_write(size_t addr, U8 value) noexcept
	{
		if (prgram_size() > 0 && prgnvram_size() > 0) [[unlikely]]
			LOG_ERROR("Not expecting cart to use both prgram and prgnvram");

		if (auto size = prgram_size();
			size > 0)
		{
			prgram_write(addr, value);
			return true;
		}

		if (auto size = prgnvram_size();
			size > 0)
		{
			prgnvram_write(addr, value);
			return true;
		}

		return false;
	}

	size_t NesCartridge::prgram_size() const noexcept
	{
		return prg_ram.size();
	}

	U8 NesCartridge::prgram_read(size_t addr) const noexcept
	{
		if (prg_ram.size() < addr) [[unlikely]]
		{
			LOG_ERROR("PRGRAM read out of range! Read from {:X}, but size is {:X}", addr, prg_nvram.size());
			return open_bus_read();
		}

		return prg_ram[addr];
	}

	bool NesCartridge::prgram_write(size_t addr, U8 value) noexcept
	{
		if (prg_ram.size() < addr) [[unlikely]]
		{
			LOG_ERROR("PRGRAM write out of range! Write to {:X}, but size is {:X}", addr, prg_nvram.size());
			return false;
		}

		prg_ram[addr] = value;
		return true;
	}

	size_t NesCartridge::prgnvram_size() const noexcept
	{
		return prg_nvram.size();
	}

	U8 NesCartridge::prgnvram_read(size_t addr) const noexcept
	{
		if (prg_nvram.size() < addr) [[unlikely]]
		{
			LOG_ERROR("PRGNVRAM read out of range! Read from {:X}, but size is {:X}", addr, prg_nvram.size());
			return open_bus_read();
		}

		return static_cast<U8>(prg_nvram[addr]);
	}

	bool NesCartridge::prgnvram_write(size_t addr, U8 value) noexcept
	{
		if (prg_nvram.size() < addr) [[unlikely]]
		{
			LOG_ERROR("PRGNVRAM write out of range! Write to {:X}, but size is {:X}", addr, prg_nvram.size());
			return false;
		}

		prg_nvram[addr] = value;
		return true;
	}

	void NesCartridge::signal_irq(bool signal) noexcept
	{
		irq_signaled = signal;
	}

	void NesCartridge::enable_bus_conflicts(bool enable) noexcept
	{
		emulate_bus_conflicts = enable;
	}

	U8 NesCartridge::open_bus_read() const noexcept
	{
		return nes->bus().open_bus_read();
	}

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
			LOG_INFO("mapper: {}", mapper(rom));
			LOG_INFO("PRG-ROM size: {0}K ({1:L})", size(rom.prg_rom) / 1024, size(rom.prg_rom));
			LOG_INFO("CHR-ROM size: {0}K ({1:L})", size(rom.chr_rom) / 1024, size(rom.chr_rom));
		}

		LOG_INFO("mirroring: {}", to_string(mirroring_mode(rom)));
		LOG_INFO("has bus conflicts: {}", has_bus_conflicts(rom));

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

		case mappers::NesMapper007::ines_mapper:
			return std::make_unique<mappers::NesMapper007>(nes, std::move(rom));

		case mappers::NesMapper009::ines_mapper:
			return std::make_unique<mappers::NesMapper009>(nes, std::move(rom));

		case mappers::NesMapper066::ines_mapper:
			return std::make_unique<mappers::NesMapper066>(nes, std::move(rom));
		}
	}
}
