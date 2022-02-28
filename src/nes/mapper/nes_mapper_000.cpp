#include "nes_mapper_000.hpp"

#include <util/logging.hpp>

namespace nesem::mapper
{
	NesMapper000::NesMapper000(NesRom &&rom) noexcept
		: rom(std::move(rom))
	{
		CHECK(rom.mapper == ines_mapper, "Wrong mapper!");
	}

	void NesMapper000::reset() noexcept
	{
		// nothing to reset
	}

	U8 NesMapper000::cpu_read(U16 addr) noexcept
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

	void NesMapper000::cpu_write([[maybe_unused]] U16 addr, [[maybe_unused]] U8 value) noexcept
	{
		// TODO: support me
		LOG_WARN("CPU write to cartridge not implemented, ignoring");
	}

	std::optional<U8> NesMapper000::ppu_read(U16 &addr) noexcept
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

	bool NesMapper000::ppu_write(U16 &addr, U8 value) noexcept
	{
		if (addr < 0x2000)
		{
			LOG_WARN("PPU write to CHR-ROM??");
			rom.chr_rom[addr] = value;
			return true;
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
