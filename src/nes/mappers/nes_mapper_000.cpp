#include "nes_mapper_000.hpp"

#include <util/logging.hpp>

namespace nesem::mappers
{
	NesMapper000::NesMapper000(const Nes &nes, NesRom &&rom_data) noexcept
		: NesCartridge(nes, std::move(rom_data))
	{
		CHECK(mapper(rom()) == ines_mapper, "Wrong mapper!");
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
				.banks{Bank{.addr = 0x8000, .bank = 0, .size = bank_16k},
					   Bank{.addr = 0xC000, .bank = 0, .size = bank_16k}},
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

	U8 NesMapper000::on_cpu_peek(U16 addr) const noexcept
	{
		if (addr < 0x6000)
		{
			LOG_ERROR("Read from invalid address ${:04X}, ignoring", addr);
			return open_bus_read();
		}

		if (addr < 0x8000)
		{
			if (auto size = cpu_ram_size();
				size > 0)
			{
				if (size > bank_8k) [[unlikely]]
					LOG_WARN("Cart has more than 8k of RAM, but we aren't doing any special bank switching? Mapper bug?");

				return cpu_ram_read(addr & (size - 1));
			}

			return open_bus_read();
		}

		U16 addr_mask = 0x7FFF;
		if (prgrom_banks(rom(), bank_16k) == 1)
			addr_mask = 0x3FFF;

		return rom().prg_rom[addr & addr_mask];
	}

	void NesMapper000::on_cpu_write([[maybe_unused]] U16 addr, [[maybe_unused]] U8 value) noexcept
	{
		if (addr < 0x6000)
		{
			LOG_ERROR("Write to invalid address ${:04X} with value {}, ignoring", addr, value);
			return;
		}

		if (addr < 0x8000)
		{
			if (auto size = cpu_ram_size();
				size > 0)
			{
				if (size > bank_8k) [[unlikely]]
					LOG_WARN("Cart has more than 8k of RAM, but we aren't doing any special bank switching? Mapper bug?");

				cpu_ram_write(addr & (size - 1), value);
			}
			return;
		}

		LOG_WARN("Write to PRG-ROM not allowed!");
	}

	std::optional<U8> NesMapper000::on_ppu_peek(U16 &addr) const noexcept
	{
		if (addr < 0x2000)
			return chr_read(addr);

		// reading from the nametable
		else if (addr < 0x3F00)
			apply_hardware_nametable_mapping(mirroring(), addr);

		return std::nullopt;
	}

	bool NesMapper000::on_ppu_write(U16 &addr, U8 value) noexcept
	{
		if (addr < 0x2000)
			return chr_write(addr, value);

		if (addr < 0x3F00)
			apply_hardware_nametable_mapping(mirroring(), addr);

		return false;
	}
}
