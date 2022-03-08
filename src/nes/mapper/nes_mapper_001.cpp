#include "nes_mapper_001.hpp"

#include <algorithm>

#include <util/logging.hpp>

namespace nesem::mapper
{
	NesMapper001::NesMapper001(NesRom &&rom) noexcept
		: rom(std::move(rom)), prg_ram(32 * 1024)
	{
		CHECK(rom.mapper == ines_mapper, "Wrong mapper!");
		reset();
	}

	void NesMapper001::reset() noexcept
	{
		load_counter = 0;
		load_shifter = 0;
		control = 0x0C;
		chr_bank_0 = 0;
		chr_bank_1 = 0;
		prg_bank_0 = 0;
		prg_bank_1 = U8(rom.prg_rom_size - 1);
	}

	U8 NesMapper001::cpu_read(U16 addr) noexcept
	{
		if (addr < 0x6000)
		{
			LOG_ERROR("Read from invalid address ${:04X}, ignoring", addr);
			return 0;
		}

		if (addr < 0x8000)
			return prg_ram[addr & 0x1FFF];

		// 16k mode
		if (control & 0b01000)
		{
			// 16k mode - low address
			if (addr < 0xC000)
				return rom.prg_rom[prg_bank_0 * 0x4000 + (addr & 0x3FFF)];

			// 16k mode - high address
			return rom.prg_rom[prg_bank_1 * 0x4000 + (addr & 0x3FFF)];
		}

		// 32k mode
		return rom.prg_rom[prg_bank_0 * 0x8000 + (addr & 0x7FFF)];
	}

	void NesMapper001::cpu_write(U16 addr, U8 value) noexcept
	{
		if (addr < 0x6000)
		{
			LOG_ERROR("Write to invalid address ${:04X} value: {:02X}, ignoring", addr, value);
			return;
		}

		if (addr < 0x8000)
		{
			prg_ram[addr & 0x1FFF] = value;
			return;
		}

		// a value with the high bit set means reset
		if (value & 0b1000'0000)
		{
			load_counter = 0;
			load_shifter = 0;
			control |= 0x0C;
			return;
		}

		// shift the lsb of value into place from left to right, that is, as load_counter increases
		// from 0-4, it shifts the lsb into the 4th, 3rd, 2nd, 1st, and 0th position
		load_shifter |= (value & 1) << load_counter;
		++load_counter;

		if (load_counter == 5)
		{
			load_complete(addr);

			// reset the shift register so we are ready to load more data
			load_counter = 0;
			load_shifter = 0;
		}
	}

	std::optional<U8> NesMapper001::ppu_read(U16 &addr) noexcept
	{
		if (addr < 0x2000)
		{
			// 4K mode
			if (control & 0b10000)
			{
				if (addr < 0x1000)
					return rom.chr_rom[chr_bank_0 * 0x1000 + (addr & 0x0FFF)];

				return rom.chr_rom[chr_bank_1 * 0x1000 + (addr & 0x0FFF)];
			}

			// 8K mode
			return rom.chr_rom[chr_bank_0 * 0x2000 + (addr & 0x1FFF)];
		}

		// reading from the nametable
		else if (addr < 0x3F00)
			nt_mirroring(addr);

		return std::nullopt;
	}

	bool NesMapper001::ppu_write(U16 &addr, U8 value) noexcept
	{
		if (addr < 0x2000)
		{
			if (rom.chr_rom_size == 0)
			{
				rom.chr_rom[addr] = value;
				return true;
			}

			LOG_WARN("PPU write to CHR-ROM??");
			return false;
		}
		else if (addr < 0x3F00)
			nt_mirroring(addr);

		return false;
	}

	void NesMapper001::nt_mirroring(U16 &addr) noexcept
	{
		auto mode = control & 3;
		switch (mode)
		{
		case 0:
		case 1:
			// one-screen, lower bank
			// one-screen, upper bank
			addr = (addr & ~0b0'000'11'00000'00000) | (mode << 10);
			break;
		case 2:
			// vertical - nothing to do
			break;
		case 3:
			// horizontal
			// exchange the nt_x and nt_y bits
			// we assume vertical mirroring by default, so this flips the 2400-27ff
			// range with the 2800-2BFF range to achieve a horizontal mirror
			addr = (addr & ~0b0'000'11'00000'00000) |
				((addr & 0b0'000'10'00000'00000) >> 1) |
				((addr & 0b0'000'01'00000'00000) << 1);
			break;
		}
	}

	void NesMapper001::load_complete(U16 addr) noexcept
	{
		if (addr < 0xA000)
		{
			control = load_shifter & 0b11111;
		}
		else if (addr < 0xC000)
		{
			// 8k mode
			if ((control & 0b10000) == 0)
			{
				// if CHR-RAM mode, we have exactly 8k so fix to the bank position
				if (rom.chr_rom_size == 0)
					chr_bank_0 = 0;

				// shifter value in 4k chunks, so ignore low bit to bring it ro 8k
				else
					chr_bank_0 = (load_shifter & 0b11110) % rom.chr_rom_size;
			}

			// 4k mode
			else
				chr_bank_0 = (load_shifter & 0b11111) % (rom.chr_rom_size * 2);
		}
		else if (addr < 0xE000)
		{
			// this is ignored in 8k mode
			if (control & 0b10000)
				chr_bank_1 = (load_shifter & 0b11111) % (rom.chr_rom_size * 2);
		}
		else // E000-FFFF
		{
			auto mode = (control >> 2) & 0x03;
			if (mode == 2)
			{
				// fix first bank at $8000 and switch 16 KB bank at $C000;
				prg_bank_0 = 0;
				prg_bank_1 = (load_shifter & 0b01111) % rom.prg_rom_size;
			}
			else if (mode == 3)
			{
				// fix last bank at $C000 and switch 16 KB bank at $8000)
				prg_bank_0 = (load_shifter & 0b01111) % rom.prg_rom_size;
				prg_bank_1 = U8(rom.prg_rom_size - 1);
			}
			else
			{
				// 32k mode - switch 32 KB at $8000, ignoring low bit of bank number
				prg_bank_0 = ((load_shifter & 0b01111) % rom.prg_rom_size) >> 1;
			}
		}
	}
}
