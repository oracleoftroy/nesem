#include "nes_mapper_002.hpp"

#include <util/logging.hpp>

namespace nesem::mappers
{
	NesMapper002::NesMapper002(NesRom &&rom) noexcept
		: rom(std::move(rom))
	{
		CHECK(rom.v1.mapper == ines_mapper, "Wrong mapper!");
	}

	void NesMapper002::reset() noexcept
	{
		bank_select = 0;
	}

	U8 NesMapper002::cpu_read(U16 addr) noexcept
	{
		if (addr < 0x8000)
		{
			LOG_ERROR("Read from invalid address ${:04X}, ignoring", addr);
			return 0;
		}

		// bank switched ROM
		if (addr < 0xC000)
			return rom.prg_rom[bank_select * 0x4000 + (addr & 0x3FFF)];

		// always fixed to the last 16K bank
		return rom.prg_rom[(prgrom_banks(rom) - 1) * 0x4000 + (addr & 0x3FFF)];
	}

	void NesMapper002::cpu_write(U16 addr, U8 value) noexcept
	{
		if (addr < 0x8000)
		{
			LOG_ERROR("Write to invalid address ${:04X}, ignoring", addr);
			return;
		}

		// writes, regardless of the address, adjust the current PRG-ROM bank we are reading from
		bank_select = (value & 0x0F) % prgrom_banks(rom);
	}

	std::optional<U8> NesMapper002::ppu_read(U16 &addr) noexcept
	{
		if (addr < 0x2000)
			return rom.chr_rom[addr];

		// reading from the nametable
		else if (addr < 0x3F00)
			apply_hardware_nametable_mapping(rom, addr);

		return std::nullopt;
	}

	bool NesMapper002::ppu_write(U16 &addr, U8 value) noexcept
	{
		if (addr < 0x2000)
		{
			if (has_chrram(rom))
			{
				rom.chr_rom[addr] = value;
				return true;
			}

			// This shouldn't happen....
			LOG_WARN("PPU write to CHR-ROM, ignoring");
		}
		else if (addr < 0x3F00)
			apply_hardware_nametable_mapping(rom, addr);

		return false;
	}
}
