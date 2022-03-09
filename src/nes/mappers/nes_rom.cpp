#include "nes_rom.hpp"

namespace nesem::mappers
{
	void apply_hardware_nametable_mapping(const NesRom &rom, U16 &addr) noexcept
	{
		if (mirroring_mode(rom) == ines_2::MirroringMode::horizontal)
		{
			// exchange the nt_x and nt_y bits
			// we assume vertical mirroring by default, so this flips the 2400-27ff
			// range with the 2800-2BFF range to achieve a horizontal mirror
			addr = (addr & ~0b0'000'11'00000'00000) |
				((addr & 0b0'000'10'00000'00000) >> 1) |
				((addr & 0b0'000'01'00000'00000) << 1);
		}
	}

	ines_2::MirroringMode mirroring_mode(const NesRom &rom) noexcept
	{
		if (rom.v2)
			return rom.v2->pcb.mirroring;

		if (rom.v1.mirror_override)
			return ines_2::MirroringMode::four_screen;

		if (rom.v1.mirroring == ines_1::Mirroring::vertical)
			return ines_2::MirroringMode::vertical;

		return ines_2::MirroringMode::horizontal;
	}

	int prgrom_banks(const NesRom &rom) noexcept
	{
		if (rom.v2)
			return int(rom.v2->prgrom.size / 16384);

		return rom.v1.prg_rom_size;
	}

	int chrrom_banks(const NesRom &rom) noexcept
	{
		if (rom.v2)
			return int(rom.v2->chrrom.has_value() ? (rom.v2->chrrom->size / 8192) : 0);

		return rom.v1.chr_rom_size;
	}

	bool has_chrram(const NesRom &rom) noexcept
	{
		if (rom.v2)
			return rom.v2->chrram.has_value();

		return rom.v1.chr_rom_size == 0;
	}

	size_t chrram_size(const NesRom &rom) noexcept
	{
		if (rom.v2)
			return rom.v2->chrram.has_value() ? rom.v2->chrram->size : 0;

		return rom.v1.chr_rom_size == 0 ? 8192 : 0;
	}

	int mapper(const NesRom &rom) noexcept
	{
		if (rom.v2)
			return rom.v2->pcb.mapper;

		return rom.v1.mapper;
	}

	int chr_banks(const NesRom &rom) noexcept
	{
		if (has_chrram(rom))
			return int(chrram_size(rom) / 8192);

		return chrrom_banks(rom);
	}
}
