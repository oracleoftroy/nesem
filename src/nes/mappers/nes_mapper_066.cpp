#include "nes_mapper_066.hpp"

#include <util/logging.hpp>

namespace nesem::mappers
{
	NesMapper066::NesMapper066(const Nes &nes, NesRom &&rom_data) noexcept
		: NesCartridge(nes, std::move(rom_data))
	{
		CHECK(rom_mapper(rom()) == ines_mapper, "Wrong mapper!");
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
			.banks{Bank{.addr = 0x0000, .bank = chr_bank_select, .size = bank_8k}}};
	}

	U8 NesMapper066::on_cpu_peek(U16 addr) const noexcept
	{
		if (addr < 0x8000)
			return open_bus_read();

		return rom().prg_rom[prg_bank_select * 0x8000 + (addr & 0x7FFF)];
	}

	void NesMapper066::on_cpu_write(U16 addr, U8 value) noexcept
	{
		if (addr < 0x8000)
			return;

		// writes, regardless of the address, adjust the current PRG-ROM and CHR-ROM bank we are reading from
		prg_bank_select = U8(((value >> 4) & 3) % rom_prgrom_banks(rom(), bank_16k));
		chr_bank_select = U8(((value >> 0) & 3) % rom_chr_banks(rom(), bank_8k));
	}

	std::optional<U8> NesMapper066::on_ppu_peek(U16 &addr) const noexcept
	{
		// return value based on the selected CHR-ROM bank
		if (addr < 0x2000)
			return chr_read(chr_bank_select * 0x2000 + addr);

		// reading from the nametable
		else if (addr < 0x3F00)
			apply_hardware_nametable_mapping(mirroring(), addr);

		return std::nullopt;
	}

	bool NesMapper066::on_ppu_write(U16 &addr, [[maybe_unused]] U8 value) noexcept
	{
		if (addr < 0x2000)
			return chr_write(chr_bank_select * 0x2000 + addr, value);
		else if (addr < 0x3F00)
			apply_hardware_nametable_mapping(mirroring(), addr);

		return false;
	}
}
