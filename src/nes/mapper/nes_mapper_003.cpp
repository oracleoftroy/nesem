#include "nes_mapper_003.hpp"

#include <util/logging.hpp>

namespace nesem::mapper
{
	NesMapper003::NesMapper003(NesRom &&rom) noexcept
		: rom(std::move(rom))
	{
		CHECK(rom.mapper == ines_mapper, "Wrong mapper!");
	}

	void NesMapper003::reset() noexcept
	{
		bank_select = 0;
	}

	U8 NesMapper003::cpu_read(U16 addr) noexcept
	{
		if (addr < 0x8000)
		{
			LOG_ERROR("Read from invalid address ${:04X}, ignoring", addr);
			return 0;
		}

		U16 addr_mask = 0x7FFF;
		if (rom.prg_rom_size == 1)
			addr_mask = 0x3FFF;

		return rom.prg_rom[addr & addr_mask];
	}

	void NesMapper003::cpu_write(U16 addr, U8 value) noexcept
	{
		if (addr < 0x8000)
		{
			LOG_ERROR_ONCE("Write to invalid address ${:04X} with value {:02X}, ignoring", addr, value);
			return;
		}

		// writes, regardless of the address, adjust the current CHR-ROM bank we are reading from
		bank_select = (value & 0x03) % rom.chr_rom_size;
	}

	std::optional<U8> NesMapper003::ppu_read(U16 &addr) noexcept
	{
		// return value based on the selected CHR-ROM bank
		if (addr < 0x2000)
			return rom.chr_rom[bank_select * 0x2000 + addr];

		// reading from the nametable
		else if (addr < 0x3F00)
			apply_hardware_nametable_mapping(rom, addr);

		return std::nullopt;
	}

	bool NesMapper003::ppu_write(U16 &addr, [[maybe_unused]] U8 value) noexcept
	{
		if (addr < 0x2000)
		{
			// This shouldn't happen....
			LOG_WARN("PPU write to CHR-ROM, ignoring");
		}
		else if (addr < 0x3F00)
			apply_hardware_nametable_mapping(rom, addr);

		return false;
	}
}