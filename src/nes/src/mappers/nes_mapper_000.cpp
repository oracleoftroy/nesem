#include "nes_mapper_000.hpp"

#include <util/logging.hpp>

namespace nesem::mappers
{
	NesMapper000::NesMapper000(const Nes &nes, NesRom &&rom_data) noexcept
		: NesCartridge(nes, std::move(rom_data))
	{
		reset();
	}

	void NesMapper000::reset() noexcept
	{
		// nothing to reset
	}

	Banks NesMapper000::report_cpu_mapping() const noexcept
	{
		auto prgrom_size = size(rom().prg_rom);
		if (prgrom_size == bank_16k)
		{
			return Banks{
				.size = 2,
				.banks{
					Bank{.addr = 0x8000, .bank = 0, .size = bank_16k},
					Bank{.addr = 0xC000, .bank = 0, .size = bank_16k},
				},
			};
		}
		else if (prgrom_size == bank_32k)
		{
			return Banks{
				.size = 1,
				.banks{Bank{.addr = 0x8000, .bank = 0, .size = bank_32k}},
			};
		}
		else
		{
			LOG_ERROR("There should only be 16k and 32k modes....");
			return {};
		}
	}

	Banks NesMapper000::report_ppu_mapping() const noexcept
	{
		return {
			.size = 1,
			.banks{Bank{.addr = 0x0000, .bank = 0, .size = bank_8k}},
		};
	}

	U8 NesMapper000::on_cpu_peek(Addr addr) const noexcept
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

				return cpu_ram_read(to_integer(addr & (size - 1)));
			}

			return open_bus_read();
		}

		U16 addr_mask = 0x7FFF;
		if (rom_prgrom_banks(rom(), bank_16k) == 1)
			addr_mask = 0x3FFF;

		return rom().prg_rom[to_integer(addr & addr_mask)];
	}

	void NesMapper000::on_cpu_write(Addr addr, U8 value) noexcept
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

				cpu_ram_write(to_integer(addr & (size - 1)), value);
			}
			return;
		}

		LOG_WARN("Write to PRG-ROM not allowed!");
	}

	std::optional<U8> NesMapper000::on_ppu_peek(Addr &addr) const noexcept
	{
		if (addr < 0x2000)
			return chr_read(to_integer(addr));

		// reading from the nametable
		else if (addr < 0x3F00)
			apply_hardware_nametable_mapping(mirroring(), addr);

		return std::nullopt;
	}

	bool NesMapper000::on_ppu_write(Addr &addr, U8 value) noexcept
	{
		if (addr < 0x2000)
			return chr_write(to_integer(addr), value);

		if (addr < 0x3F00)
			apply_hardware_nametable_mapping(mirroring(), addr);

		return false;
	}
}
