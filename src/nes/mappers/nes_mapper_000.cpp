#include "nes_mapper_000.hpp"

#include <util/logging.hpp>

namespace nesem::mappers
{
	NesMapper000::NesMapper000(const Nes &nes, NesRom &&rom_data) noexcept
		: NesCartridge(nes, std::move(rom_data))
	{
		CHECK(rom.v1.mapper == ines_mapper, "Wrong mapper!");
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
		if (prgrom_banks(rom, bank_16k) == 1)
			addr_mask = 0x3FFF;

		return rom.prg_rom[addr & addr_mask];
	}

	void NesMapper000::cpu_write([[maybe_unused]] U16 addr, [[maybe_unused]] U8 value) noexcept
	{
		if (has_chrram(rom))
		{
			rom.chr_rom[addr] = value;
			return;
		}

		// NOTE: Mapper 000 doesn't support writing at all, as far as I am aware, but some games do seem to write
		// in the range of the cartridge (e.g. Mrs. Pac Man / Ice Climber). There is no obvious error from ignoring
		// the write, and Nesdev indicates that it is due to bus conflicts, but I don't know what other component
		// would be listening within that address range.
		// TODO: Is there any valid circumstance where we would need to support writing?
		LOG_WARN_ONCE("CPU write to cartridge, ignoring... addr: ${:04X} value: ${:02X}", addr, value);
		LOG_WARN_ONCE("***All future writes will be ignored***");
	}

	std::optional<U8> NesMapper000::ppu_read(U16 &addr) noexcept
	{
		if (addr < 0x2000)
			return rom.chr_rom[addr];

		// reading from the nametable
		else if (addr < 0x3F00)
			apply_hardware_nametable_mapping(rom, addr);

		return std::nullopt;
	}

	bool NesMapper000::ppu_write(U16 &addr, U8 value) noexcept
	{
		if (addr < 0x2000)
		{
			if (has_chrram(rom))
			{
				rom.chr_rom[addr] = value;
				return true;
			}

			LOG_WARN("PPU write to CHR-ROM??");
		}
		else if (addr < 0x3F00)
			apply_hardware_nametable_mapping(rom, addr);

		return false;
	}
}
