#include "nes_mapper_002.hpp"

#include <util/logging.hpp>

namespace nesem::mappers
{
	NesMapper002::NesMapper002(const Nes &nes, NesRom &&rom_data) noexcept
		: NesCartridge(nes, std::move(rom_data))
	{
		CHECK(mapper(rom()) == ines_mapper, "Wrong mapper!");
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

	Banks NesMapper002::report_ppu_mapping() const noexcept
	{
		return {
			.size = 1,
			.banks = Bank{.addr = 0x0000, .bank = 0, .size = bank_8k}
        };
	}

	U8 NesMapper002::on_cpu_peek(U16 addr) const noexcept
	{
		if (addr < 0x8000)
		{
			LOG_ERROR("Read from invalid address ${:04X}, ignoring", addr);
			return open_bus_read();
		}

		// bank switched ROM
		if (addr < 0xC000)
			return rom().prg_rom[bank_select * bank_16k + (addr & 0x3FFF)];

		// always fixed to the last 16K bank
		return rom().prg_rom[(prgrom_banks(rom(), bank_16k) - 1) * bank_16k + (addr & (bank_16k - 1))];
	}

	void NesMapper002::on_cpu_write(U16 addr, U8 value) noexcept
	{
		if (addr < 0x8000)
		{
			LOG_ERROR_ONCE("Write to invalid address ${:04X}, ignoring", addr);
			return;
		}

		// writes, regardless of the address, adjust the current PRG-ROM bank we are reading from
		bank_select = U8((value & 0x0F) % prgrom_banks(rom(), bank_16k));
	}

	std::optional<U8> NesMapper002::on_ppu_peek(U16 &addr) const noexcept
	{
		if (addr < 0x2000)
			return chr_read(addr);

		// reading from the nametable
		else if (addr < 0x3F00)
			apply_hardware_nametable_mapping(mirroring(), addr);

		return std::nullopt;
	}

	bool NesMapper002::on_ppu_write(U16 &addr, U8 value) noexcept
	{
		if (addr < 0x2000)
			return chr_write(addr, value);

		else if (addr < 0x3F00)
			apply_hardware_nametable_mapping(mirroring(), addr);

		return false;
	}
}
