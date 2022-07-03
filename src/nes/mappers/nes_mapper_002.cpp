#include "nes_mapper_002.hpp"

#include <util/logging.hpp>

namespace nesem::mappers
{
	NesMapper002::NesMapper002(const Nes &nes, NesRom &&rom_data) noexcept
		: NesCartridge(nes, std::move(rom_data))
	{
		CHECK(rom().v1.mapper == ines_mapper, "Wrong mapper!");
	}

	void NesMapper002::reset() noexcept
	{
		bank_select = 0;
	}

	Banks NesMapper002::report_cpu_mapping() const noexcept
	{
		auto last_bank = U16(prgrom_banks(rom(), bank_16k) - 1);
		return {
			.size = 2,
			.banks = {Bank{.addr = 0x8000, .bank = bank_select, .size = bank_16k},
					  Bank{.addr = 0xC000, .bank = last_bank, .size = bank_16k}}
        };
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
			return rom().prg_rom[bank_select * bank_16k + (addr & 0x3FFF)];

		// always fixed to the last 16K bank
		return rom().prg_rom[(prgrom_banks(rom(), bank_16k) - 1) * bank_16k + (addr & (bank_16k - 1))];
	}

	void NesMapper002::cpu_write(U16 addr, U8 value) noexcept
	{
		if (addr < 0x8000)
		{
			LOG_ERROR("Write to invalid address ${:04X}, ignoring", addr);
			return;
		}

		// writes, regardless of the address, adjust the current PRG-ROM bank we are reading from
		bank_select = U8((value & 0x0F) % prgrom_banks(rom(), bank_16k));
	}

	std::optional<U8> NesMapper002::ppu_read(U16 &addr) noexcept
	{
		if (addr < 0x2000)
			return chr_read(addr);

		// reading from the nametable
		else if (addr < 0x3F00)
			apply_hardware_nametable_mapping(mirroring(), addr);

		return std::nullopt;
	}

	bool NesMapper002::ppu_write(U16 &addr, U8 value) noexcept
	{
		if (addr < 0x2000)
		{
			return chr_write(addr, value);
		}
		else if (addr < 0x3F00)
			apply_hardware_nametable_mapping(mirroring(), addr);

		return false;
	}
}
