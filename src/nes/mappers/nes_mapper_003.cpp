#include "nes_mapper_003.hpp"

#include <util/logging.hpp>

namespace nesem::mappers
{
	NesMapper003::NesMapper003(const Nes &nes, NesRom &&rom_data) noexcept
		: NesCartridge(nes, std::move(rom_data))
	{
		reset();
	}

	void NesMapper003::reset() noexcept
	{
		bank_select = 0;
	}

	Banks NesMapper003::report_cpu_mapping() const noexcept
	{
		if (rom_prgrom_banks(rom(), bank_16k) == 1)
		{
			return {
				.size = 2,
				.banks{Bank{.addr = 0x8000, .bank = 0, .size = bank_16k},
					   Bank{.addr = 0xC000, .bank = 0, .size = bank_16k}}
            };
		}
		return {
			.size = 1,
			.banks{Bank{.addr = 0x8000, .bank = 0, .size = bank_32k}}};
	}

	Banks NesMapper003::report_ppu_mapping() const noexcept
	{
		return {
			.size = 1,
			.banks{Bank{.addr = 0x0000, .bank = bank_select, .size = bank_8k}}};
	}

	U8 NesMapper003::on_cpu_peek(Addr addr) const noexcept
	{
		if (addr < 0x6000)
			return open_bus_read();

		if (addr < 0x8000)
		{
			if (auto size = cpu_ram_size();
				size > 0)
			{
				if (size > bank_8k) [[unlikely]]
					LOG_WARN("Cart has more than 8k of RAM, but we aren't doing any special bank switching? Mapper bug?");

				return cpu_ram_read(to_rom_addr(0, size, addr));
			}

			return open_bus_read();
		}

		U16 addr_mask = 0x7FFF;
		if (rom_prgrom_banks(rom(), bank_16k) == 1)
			addr_mask = 0x3FFF;

		return rom().prg_rom[to_integer(addr & addr_mask)];
	}

	void NesMapper003::on_cpu_write(Addr addr, U8 value) noexcept
	{
		if (addr < 0x6000)
			return;

		if (addr < 0x8000)
		{
			if (auto size = cpu_ram_size();
				size > 0)
			{
				if (size > bank_8k) [[unlikely]]
					LOG_WARN("Cart has more than 8k of RAM, but we aren't doing any special bank switching? Mapper bug?");

				cpu_ram_write(to_rom_addr(0, size, addr), value);
			}
			return;
		}

		// writes, regardless of the address, adjust the current CHR-ROM bank we are reading from
		bank_select = U8((value & 0x03) % rom_chr_banks(rom(), bank_8k));
	}

	std::optional<U8> NesMapper003::on_ppu_peek(Addr &addr) const noexcept
	{
		// return value based on the selected CHR-ROM bank
		if (addr < 0x2000)
			return chr_read(to_rom_addr(bank_select, bank_8k, addr));

		// reading from the nametable
		else if (addr < 0x3F00)
			apply_hardware_nametable_mapping(mirroring(), addr);

		return std::nullopt;
	}

	bool NesMapper003::on_ppu_write(Addr &addr, U8 value) noexcept
	{
		if (addr < 0x2000)
			return chr_write(to_rom_addr(bank_select, bank_8k, addr), value);

		else if (addr < 0x3F00)
			apply_hardware_nametable_mapping(mirroring(), addr);

		return false;
	}
}
