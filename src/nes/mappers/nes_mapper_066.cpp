#include "nes_mapper_066.hpp"

#include <util/logging.hpp>

namespace nesem::mappers
{
	NesMapper066::NesMapper066(const Nes &nes, NesRom &&rom_data) noexcept
		: NesCartridge(nes, std::move(rom_data))
	{
		CHECK(rom().v1.mapper == ines_mapper, "Wrong mapper!");
	}

	void NesMapper066::reset() noexcept
	{
		prg_bank_select = 0;
		chr_bank_select = 0;
	}

	Banks NesMapper066::report_cpu_mapping() const noexcept
	{
		// always has a single active 32k bank
		return {
			.size = 1,
			.banks{Bank{.addr = 0x8000, .bank = prg_bank_select, .size = bank_32k}}};
	}

	Banks NesMapper066::report_ppu_mapping() const noexcept
	{
		return {
			.size = 1,
			.banks = Bank{.addr = 0x0000, .bank = chr_bank_select, .size = bank_8k}
        };
	}

	U8 NesMapper066::cpu_read(U16 addr) noexcept
	{
		if (addr < 0x8000)
		{
			LOG_ERROR("Read from invalid address ${:04X}, ignoring", addr);
			return 0;
		}

		return rom().prg_rom[prg_bank_select * 0x8000 + (addr & 0x7FFF)];
	}

	void NesMapper066::cpu_write(U16 addr, U8 value) noexcept
	{
		if (addr < 0x8000)
		{
			LOG_ERROR_ONCE("Write to invalid address ${:04X} with value {:02X}, ignoring", addr, value);
			return;
		}

		// writes, regardless of the address, adjust the current PRG-ROM and CHR-ROM bank we are reading from
		prg_bank_select = U8(((value >> 4) & 3) % prgrom_banks(rom(), bank_16k));
		chr_bank_select = U8(((value >> 0) & 3) % chr_banks(rom(), bank_8k));
	}

	std::optional<U8> NesMapper066::ppu_read(U16 &addr) noexcept
	{
		// return value based on the selected CHR-ROM bank
		if (addr < 0x2000)
			return chr_read(chr_bank_select * 0x2000 + addr);

		// reading from the nametable
		else if (addr < 0x3F00)
			apply_hardware_nametable_mapping(mirroring(), addr);

		return std::nullopt;
	}

	bool NesMapper066::ppu_write(U16 &addr, [[maybe_unused]] U8 value) noexcept
	{
		if (addr < 0x2000)
			return chr_write(chr_bank_select * 0x2000 + addr, value);
		else if (addr < 0x3F00)
			apply_hardware_nametable_mapping(mirroring(), addr);

		return false;
	}
}
