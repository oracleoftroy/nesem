#include "nes_mapper_001.hpp"

#include <algorithm>

#include <util/logging.hpp>

namespace nesem::mappers
{
	NesMapper001::NesMapper001(NesRom &&rom) noexcept
		: rom(std::move(rom))
	{
		CHECK(rom.mapper == ines_mapper, "Wrong mapper!");
		reset();
	}

	void NesMapper001::reset() noexcept
	{
		load_counter = 0;
		load_shifter = 0;
		reg.control = 0x0C;
		reg.chr_0 = 0;
		reg.chr_1 = 0;
		reg.chr_last = 0;
		reg.prg = 0;

		chr_bank_0 = 0;
		chr_bank_1 = 0;
		prg_bank_0 = 0;
		prg_bank_1 = U8(prgrom_banks(rom) - 1);

		if (rom.v2)
		{
			size_t prg_ram_size = 0;
			if (rom.v2->prgram)
				prg_ram_size += rom.v2->prgram->size;

			if (rom.v2->prgnvram)
				prg_ram_size += rom.v2->prgnvram->size;

			if (prg_ram_size > 0x2000)
				LOG_WARN("Allocating more than 8k RAM, but we currently don't support handling that much!");

			prg_ram.resize(prg_ram_size);
		}
		else
		{
			// default to 32k.. why? because NesDev said it was a good default... but we only write here if in the 8k range of 6000-7FFF?
			prg_ram.resize(32 * 1024);
		}

		update_state();
	}

	U8 NesMapper001::cpu_read(U16 addr) noexcept
	{
		if (addr < 0x6000)
		{
			LOG_ERROR("Read from invalid address ${:04X}, ignoring", addr);
			return 0;
		}

		if (addr < 0x8000)
		{
			// TODO: some carts have both battery backed and non-battery backed ram and need special handling of chr for bank switching between them
			// currently we are treating all ram the same, but at some point we need to properly handle battery backed ram and savestates
			if (prg_ram_enabled)
				return prg_ram[addr & 0x1FFF];
			else
			{
				LOG_WARN("PRG-RAM write while disabled?");
				return 0;
			}
		}

		if (prg_mode == Prg::size_16k)
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
			reg.control |= 0x0C;
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
			if (chr_mode == Chr::size_4k)
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
			if (has_chrram(rom))
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
		auto mode = reg.control & 3;
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
			reg.control = load_shifter & 0b11111;
		else if (addr < 0xC000)
			reg.chr_last = reg.chr_0 = load_shifter & 0b11111;
		else if (addr < 0xE000)
			reg.chr_last = reg.chr_1 = load_shifter & 0b11111;
		else // E000-FFFF
			reg.prg = load_shifter & 0b11111;

		update_state();
	}

	void NesMapper001::update_state() noexcept
	{
		prg_ram_enabled = (reg.prg & 0b10000) == 0;

		prg_mode = (reg.control & 0b01000) ? Prg::size_16k : Prg::size_32k;
		chr_mode = (reg.control & 0b10000) ? Chr::size_4k : Chr::size_8k;

		// update CHR-ROM banks
		if (chr_mode == Chr::size_8k)
		{
			// 8k mode
			// shifter value in 4k chunks, so ignore low bit to bring it to 8k
			chr_bank_0 = (reg.chr_0 & 0b11110) % chr_banks(rom);
		}
		else
		{
			chr_bank_0 = reg.chr_0 % (chr_banks(rom) * 2);
			chr_bank_1 = reg.chr_1 % (chr_banks(rom) * 2);
		}

		// calculate the prg_bank we will use. By default, it uses the first 4 bits written to prg,
		// but larger carts use extra bits written to the chr registers
		U8 prg_bank = reg.prg & 0xF;

		// 512k PRG-ROM (32 banks @16k each)
		if (prgrom_banks(rom) == 32)
		{
			auto high_bank = chr_mode == Chr::size_4k ? reg.chr_last : reg.chr_0;

			// 512kb carts use bit 5 of chr reg to select page
			prg_bank |= (high_bank & 0x10);
		}

		// update PRG-ROM
		if (prg_mode == Prg::size_16k)
		{
			auto mode = (reg.control >> 2) & 0x01;
			if (mode == 0)
			{
				// fix first bank at $8000 and switch 16 KB bank at $C000;
				prg_bank_0 = 0;
				prg_bank_1 = prg_bank % prgrom_banks(rom);
			}
			else if (mode == 1)
			{
				// fix last bank at $C000 and switch 16 KB bank at $8000)
				prg_bank_0 = prg_bank % prgrom_banks(rom);
				prg_bank_1 = U8(prgrom_banks(rom) - 1);
			}
		}
		else
		{
			// 32k mode - switch 32 KB at $8000, ignoring low bit of bank number
			prg_bank_0 = (prg_bank % prgrom_banks(rom)) >> 1;
		}
	}
}
