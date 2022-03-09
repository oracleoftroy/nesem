#include "nes_mapper_066.hpp"

#include <util/logging.hpp>

namespace nesem::mappers
{
	NesMapper066::NesMapper066(NesRom &&rom) noexcept
		: rom(std::move(rom))
	{
		CHECK(rom.mapper == ines_mapper, "Wrong mapper!");
	}

	void NesMapper066::reset() noexcept
	{
		prg_bank_select = 0;
		chr_bank_select = 0;
	}

	U8 NesMapper066::cpu_read(U16 addr) noexcept
	{
		if (addr < 0x8000)
		{
			LOG_ERROR("Read from invalid address ${:04X}, ignoring", addr);
			return 0;
		}

		return rom.prg_rom[prg_bank_select * 0x8000 + (addr & 0x7FFF)];
	}

	void NesMapper066::cpu_write(U16 addr, U8 value) noexcept
	{
		if (addr < 0x8000)
		{
			LOG_ERROR_ONCE("Write to invalid address ${:04X} with value {:02X}, ignoring", addr, value);
			return;
		}

		// writes, regardless of the address, adjust the current PRG-ROM and CHR-ROM bank we are reading from
		prg_bank_select = ((value >> 4) & 3) % prgrom_banks(rom);
		chr_bank_select = ((value >> 0) & 3) % chr_banks(rom);
	}

	std::optional<U8> NesMapper066::ppu_read(U16 &addr) noexcept
	{
		// return value based on the selected CHR-ROM bank
		if (addr < 0x2000)
			return rom.chr_rom[chr_bank_select * 0x2000 + addr];

		// reading from the nametable
		else if (addr < 0x3F00)
			apply_hardware_nametable_mapping(rom, addr);

		return std::nullopt;
	}

	bool NesMapper066::ppu_write(U16 &addr, [[maybe_unused]] U8 value) noexcept
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
