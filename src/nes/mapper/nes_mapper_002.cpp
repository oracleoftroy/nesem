#include "nes_mapper_002.hpp"

#include <util/logging.hpp>

namespace nesem::mapper
{
	NesMapper002::NesMapper002(NesRom &&rom) noexcept
		: rom(std::move(rom))
	{
		CHECK(rom.mapper == ines_mapper, "Wrong mapper!");
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
		return rom.prg_rom[(rom.prg_rom_size - 1) * 0x4000 + (addr & 0x3FFF)];
	}

	void NesMapper002::cpu_write(U16 addr, U8 value) noexcept
	{
		if (addr < 0x8000)
		{
			LOG_ERROR("Write to invalid address ${:04X}, ignoring", addr);
			return;
		}

		// writes, regardless of the address, adjust the current PRG-ROM bank we are reading from
		bank_select = (value & 0x0F) % rom.prg_rom_size;
	}

	std::optional<U8> NesMapper002::ppu_read(U16 &addr) noexcept
	{
		if (addr < 0x2000)
			return rom.chr_rom[addr];

		// reading from the nametable
		else if (addr < 0x3F00)
		{
			if (rom.mirroring == NesMirroring::horizontal)
			{
				// exchange the nt_x and nt_y bits
				// we assume vertical mirroring by default, so this flips the 2400-27ff
				// range with the 2800-2BFF range to achieve a horizontal mirror
				addr = (addr & ~0b0'000'11'00000'00000) |
					((addr & 0b0'000'10'00000'00000) >> 1) |
					((addr & 0b0'000'01'00000'00000) << 1);
			}
		}

		return std::nullopt;
	}

	bool NesMapper002::ppu_write(U16 &addr, U8 value) noexcept
	{
		if (addr < 0x2000)
		{
			// CHR-ROM size of 0 indicates 8k of CHR-RAM
			if (rom.chr_rom_size == 0)
			{
				rom.chr_rom[addr] = value;
				return true;
			}

			// This shouldn't happen....
			LOG_WARN("PPU write to CHR-ROM, ignoring");
		}
		else if (addr < 0x3F00)
		{
			if (rom.mirroring == NesMirroring::horizontal)
			{
				// exchange the nt_x and nt_y bits
				// we assume vertical mirroring by default, so this flips the 2400-27ff
				// range with the 2800-2BFF range to achieve a horizontal mirror
				addr = (addr & ~0b0'000'11'00000'00000) |
					((addr & 0b0'000'10'00000'00000) >> 1) |
					((addr & 0b0'000'01'00000'00000) << 1);
			}
		}

		return false;
	}
}
