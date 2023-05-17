#include "nes_mapper_007.hpp"

#include <util/logging.hpp>

namespace nesem::mappers
{
	NesMapper007::NesMapper007(const Nes &nes, NesRom &&rom_data) noexcept
		: NesCartridge(nes, std::move(rom_data))
	{
		reset();
	}

	void NesMapper007::reset() noexcept
	{
		num_banks = U8(rom_prgrom_banks(rom(), bank_32k));
	}

	Banks NesMapper007::report_cpu_mapping() const noexcept
	{
		U16 bank = bank_select & 0x0F;
		bank &= num_banks - 1;

		return {
			.size = 1,
			.banks{Bank{.addr = 0x8000, .bank = bank, .size = bank_32k}},
		};
	}

	Banks NesMapper007::report_ppu_mapping() const noexcept
	{
		return {
			.size = 1,
			.banks{Bank{.addr = 0x0000, .bank = 0, .size = bank_8k}},
		};
	}

	MirroringMode NesMapper007::mirroring() const noexcept
	{
		if (bank_select & 0x10)
			return MirroringMode::four_screen;
		else
			return MirroringMode::one_screen;
	}

	U8 NesMapper007::on_cpu_peek(Addr addr) const noexcept
	{
		if (addr < 0x6000)
			return open_bus_read();

		// prg-ram
		if (addr < 0x8000)
		{
			LOG_ERROR_ONCE("Mapper 007 doesn't have PRG-RAM... read from addr: {}", addr);
			return open_bus_read();
		}

		// extension, official carts only use the first 3 bits, but some roms use all 4
		auto bank = bank_select & 0x0F;
		bank &= num_banks - 1;

		return rom().prg_rom[to_rom_addr(bank, bank_32k, addr)];
	}

	void NesMapper007::on_cpu_write(Addr addr, U8 value) noexcept
	{
		if (addr < 0x6000)
			return;

		// prg-ram
		if (addr < 0x8000)
			LOG_ERROR_ONCE("Mapper 007 doesn't have PRG-RAM... write to addr: {} value: {:02X}", addr, value);

		bank_select = value;
	}

	std::optional<U8> NesMapper007::on_ppu_peek(Addr &addr) const noexcept
	{
		if (addr < 0x2000)
			return chr_read(to_integer(addr));

		// reading from the nametable
		else if (addr < 0x3F00)
			apply_hardware_nametable_mapping(mirroring(), addr);

		return std::nullopt;
	}

	bool NesMapper007::on_ppu_write(Addr &addr, U8 value) noexcept
	{
		if (addr < 0x2000)
			return chr_write(to_integer(addr), value);

		else if (addr < 0x3F00)
			apply_hardware_nametable_mapping(mirroring(), addr);

		return false;
	}
}
