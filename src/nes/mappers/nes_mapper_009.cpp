#include "nes_mapper_009.hpp"

#include <util/logging.hpp>

namespace nesem::mappers
{
	NesMapper009::NesMapper009(const Nes &nes, NesRom &&rom_data) noexcept
		: NesCartridge(nes, std::move(rom_data))
	{
		CHECK(mapper(rom()) == ines_mapper, "Wrong mapper!");

		reset();
	}

	void NesMapper009::reset() noexcept
	{
		prgrom_bank = 0;
		chr_0_fd = 0;
		chr_0_fe = 0;
		chr_1_fd = 0;
		chr_1_fe = 0;
	}

	Banks NesMapper009::report_cpu_mapping() const noexcept
	{
		const auto num_banks = U16(prgrom_banks(rom(), bank_8k));

		return {
			.size = 4,
			.banks{Bank{.addr = 0x8000, .bank = prgrom_bank, .size = bank_8k},
				   Bank{.addr = 0xA000, .bank = num_banks - 3, .size = bank_8k},
				   Bank{.addr = 0xC000, .bank = num_banks - 2, .size = bank_8k},
				   Bank{.addr = 0xE000, .bank = num_banks - 1, .size = bank_8k}}
        };
	}

	Banks NesMapper009::report_ppu_mapping() const noexcept
	{
		auto bank_0 = chr_0 ? chr_0_fe : chr_0_fd;
		auto bank_1 = chr_1 ? chr_1_fe : chr_1_fd;

		return {
			.size = 2,
			.banks{Bank{.addr = 0x0000, .bank = bank_0, .size = bank_4k},
				   Bank{.addr = 0x1000, .bank = bank_1, .size = bank_4k}}
        };
	}

	MirroringMode NesMapper009::mirroring() const noexcept
	{
		if (mirror & 1)
			return MirroringMode::horizontal;

		return MirroringMode::vertical;
	}

	U8 NesMapper009::on_cpu_peek(U16 addr) const noexcept
	{
		if (addr < 0x6000)
		{
			LOG_ERROR("Read from invalid address ${:04X}, ignoring", addr);
			return open_bus_read();
		}

		// prg-ram
		if (addr < 0x8000)
		{
			if (cpu_ram_size() > 0)
				return cpu_ram_read(addr & (bank_8k - 1));
			else
			{
				LOG_ERROR("Read from invalid address ${:04X}, ignoring", addr);
				return open_bus_read();
			}
		}

		auto bank = prgrom_banks(rom(), bank_8k);

		// bank-switched
		if (addr < 0xA000)
			bank = prgrom_bank;

		// fixed to last 24K, treated as 3 8K banks
		else if (addr < 0xC000)
			bank -= 3;

		else if (addr < 0xE000)
			bank -= 2;

		else
			bank -= 1;

		return rom().prg_rom[bank * bank_8k + (addr & (bank_8k - 1))];
	}

	void NesMapper009::on_cpu_write(U16 addr, U8 value) noexcept
	{
		if (addr < 0x6000)
		{
			LOG_ERROR("Write to invalid address ${:04X} with value {:02X}, ignoring", addr, value);
			return;
		}

		if (addr < 0x8000)
		{
			if (cpu_ram_size() > 0)
				cpu_ram_write(addr & (bank_8k - 1), value);
			else
				LOG_ERROR("Write to invalid address ${:04X} with value {:02X}, ignoring", addr, value);

			return;
		}

		if (addr < 0xA000)
		{
			LOG_ERROR("Write to invalid address ${:04X} with value {:02X}, ignoring", addr, value);
			return;
		}

		// everything from $A000-FFFF is mapped to internal registers
		switch (addr & 0xF000)
		{
		case 0xA000:
			// prgrom_bank = U8(value % prgrom_banks(rom(), bank_8k));
			prgrom_bank = value & 0b0000'1111;
			break;
		case 0xB000:
			chr_0_fd = value & 0b0001'1111;
			break;
		case 0xC000:
			chr_0_fe = value & 0b0001'1111;
			break;
		case 0xD000:
			chr_1_fd = value & 0b0001'1111;
			break;
		case 0xE000:
			chr_1_fe = value & 0b0001'1111;
			break;
		case 0xF000:
			mirror = value & 0b0000'0001;
			break;
		}
	}

	std::optional<U8> NesMapper009::on_ppu_peek(U16 &addr) const noexcept
	{
		if (addr < 0x1000)
		{
			auto bank = chr_0 ? chr_0_fe : chr_0_fd;
			return chr_read(bank * bank_4k + (addr & (bank_4k - 1)));
		}

		if (addr < 0x2000)
		{
			auto bank = chr_1 ? chr_1_fe : chr_1_fd;
			return chr_read(bank * bank_4k + (addr & (bank_4k - 1)));
		}

		// reading from the nametable
		if (addr < 0x3F00)
			apply_hardware_nametable_mapping(mirroring(), addr);

		return std::nullopt;
	}

	std::optional<U8> NesMapper009::on_ppu_read(U16 &addr) noexcept
	{
		auto value = on_ppu_peek(addr);

		// bank switching CHR-ROM is based on reads to specific addresses, the switch happens after the read

		// PPU reads $0FD8: latch 0 is set to $FD for subsequent reads
		if (addr == 0x0FD8)
			chr_0 = false;

		// PPU reads $0FE8: latch 0 is set to $FE for subsequent reads
		else if (addr == 0x0FE8)
			chr_0 = true;

		// PPU reads $1FD8 through $1FDF: latch 1 is set to $FD for subsequent reads
		else if (addr >= 0x1FD8 && addr <= 0x1FDF)
			chr_1 = false;

		// PPU reads $1FE8 through $1FEF: latch 1 is set to $FE for subsequent reads
		else if (addr >= 0x1FE8 && addr <= 0x1FEF)
			chr_1 = true;

		return value;
	}

	bool NesMapper009::on_ppu_write(U16 &addr, U8 value) noexcept
	{
		if (addr < 0x2000)
			return chr_write(addr, value);

		else if (addr < 0x3F00)
			apply_hardware_nametable_mapping(mirroring(), addr);

		return false;
	}
}
