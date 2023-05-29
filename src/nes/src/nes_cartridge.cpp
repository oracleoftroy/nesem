#include "nes_cartridge.hpp"

#include <utility>

#include "nes.hpp"
#include "nes_cartridge_loader.hpp"

#include <util/logging.hpp>

namespace nesem
{
	NesCartridge::NesCartridge(const Nes &nes, mappers::NesRom &&rom_data) noexcept
		: nes(&nes), nes_rom(std::move(rom_data))
	{
		if (rom_has_chrram(rom()))
		{
			if (!rom().chr_rom.empty())
				LOG_WARN("CHR-ROM not empty, but we assume CHR-ROM and CHR-RAM are mutually exclusive!");

			chr_ram.resize(rom_chrram_size(rom()));
		}

		if (rom().v2)
		{
			if (rom().v2->prgram)
				prg_ram.resize(rom().v2->prgram.value());

			if (rom().v2->prgnvram)
				prg_nvram = nes.open_prgnvram(rom().v2->rom.sha1, rom().v2->prgnvram.value());
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

		emulate_bus_conflicts = rom_has_bus_conflicts(nes_rom);
	}

	mappers::MirroringMode NesCartridge::mirroring() const noexcept
	{
		// default implementation returns whatever the ROM tells us
		return rom_mirroring_mode(nes_rom);
	}

	U8 NesCartridge::cpu_peek(Addr addr) const noexcept
	{
		return on_cpu_peek(addr);
	}

	U8 NesCartridge::cpu_read(Addr addr) noexcept
	{
		signal_m2(true);
		return on_cpu_read(addr);
	}

	void NesCartridge::cpu_write(Addr addr, U8 value) noexcept
	{
		signal_m2(true);

		if (emulate_bus_conflicts)
			value &= cpu_peek(addr);

		on_cpu_write(addr, value);
	}

	std::optional<U8> NesCartridge::ppu_peek(Addr &addr) const noexcept
	{
		if (addr >= 0x4000) [[unlikely]]
			LOG_ERROR("address out of range? PPU should properly mirror addresses, but we got ${}", addr);

		return on_ppu_peek(addr);
	}

	std::optional<U8> NesCartridge::ppu_read(Addr &addr) noexcept
	{
		if (addr >= 0x4000) [[unlikely]]
			LOG_ERROR("address out of range? PPU should properly mirror addresses, but we got ${}", addr);

		return on_ppu_read(addr);
	}

	bool NesCartridge::ppu_write(Addr &addr, U8 value) noexcept
	{
		if (addr >= 0x4000) [[unlikely]]
			LOG_ERROR("address out of range? PPU should properly mirror addresses, but we got ${}", addr);

		return on_ppu_write(addr, value);
	}

	U8 NesCartridge::on_cpu_read(Addr addr) noexcept
	{
		return on_cpu_peek(addr);
	}

	std::optional<U8> NesCartridge::on_ppu_read(Addr &addr) noexcept
	{
		return on_ppu_peek(addr);
	}

	const mappers::NesRom &NesCartridge::rom() const noexcept
	{
		return nes_rom;
	}

	bool NesCartridge::irq() const noexcept
	{
		return irq_signaled;
	}

	size_t NesCartridge::chr_size() const noexcept
	{
		if (rom_has_chrram(nes_rom))
			return size(chr_ram);

		return size(nes_rom.chr_rom);
	}

	void NesCartridge::signal_m2([[maybe_unused]] bool rising) noexcept
	{
		// default, do nothing
	}

	U8 NesCartridge::chr_read(size_t addr) const noexcept
	{
		if (rom_has_chrram(nes_rom))
			return chr_ram[addr];

		return nes_rom.chr_rom[addr];
	}

	bool NesCartridge::chr_write(size_t addr, U8 value) noexcept
	{
		if (rom_has_chrram(nes_rom))
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

		return prg_nvram[addr];
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
		if (irq_signaled != signal)
			LOG_DEBUG("IRQ {} on PPU scanline {}, cycle {}", (signal ? "signaled" : "cleared"), nes->ppu().current_scanline(), nes->ppu().current_cycle());

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
}
