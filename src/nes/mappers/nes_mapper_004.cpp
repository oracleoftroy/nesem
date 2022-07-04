#include "nes_mapper_004.hpp"

#include <utility>

#include "nes.hpp"

#include <util/logging.hpp>

namespace nesem::mappers
{
	NesMapper004::NesMapper004(const Nes &nes, NesRom &&rom_data) noexcept
		: NesCartridge(nes, std::move(rom_data))
	{
		CHECK(mapper(rom()) == ines_mapper, "Wrong mapper!");

		prg_ram.resize(bank_8k);
		reset();
	}

	void NesMapper004::reset() noexcept
	{
		bank_select = 0;
		bank_map = {};

		mirror = 0;
		prg_ram_protect = 0;

		irq_latch = 255;
		irq_reload = false;
		irq_counter = 255;

		irq_enabled = false;
		signal_irq(false);

		a12 = 1;
		cycle_low = 0;
	}

	Banks NesMapper004::report_cpu_mapping() const noexcept
	{
		int mode = (bank_select >> 6) & 1;
		auto num_banks = prgrom_banks(rom(), bank_8k);

		U16 bank0 = bank_map[6];
		U16 bank1 = bank_map[7];
		U16 bank2 = num_banks - 2;
		U16 bank3 = num_banks - 1;

		if (mode == 1)
			std::swap(bank0, bank2);

		return {
			.size = 4,
			.banks = {
					  Bank{.addr = 0x8000, .bank = bank0, .size = bank_8k},
					  Bank{.addr = 0xA000, .bank = bank1, .size = bank_8k},
					  Bank{.addr = 0xC000, .bank = bank2, .size = bank_8k},
					  Bank{.addr = 0xE000, .bank = bank3, .size = bank_8k},
					  }
        };
	}

	Banks NesMapper004::report_ppu_mapping() const noexcept
	{
		auto mode = (bank_select >> 7) & 1;

		if (mode == 0)
		{
			return {
				.size = 8,
				.banks = {
						  Bank{.addr = 0x0000, .bank = U16(bank_map[0] + 0), .size = bank_1k},
						  Bank{.addr = 0x0400, .bank = U16(bank_map[0] + 1), .size = bank_1k},
						  Bank{.addr = 0x0800, .bank = U16(bank_map[1] + 0), .size = bank_1k},
						  Bank{.addr = 0x0C00, .bank = U16(bank_map[1] + 1), .size = bank_1k},
						  Bank{.addr = 0x1000, .bank = U16(bank_map[2]), .size = bank_1k},
						  Bank{.addr = 0x1400, .bank = U16(bank_map[3]), .size = bank_1k},
						  Bank{.addr = 0x1800, .bank = U16(bank_map[4]), .size = bank_1k},
						  Bank{.addr = 0x1C00, .bank = U16(bank_map[5]), .size = bank_1k},
						  }
            };
		}
		else
		{
			return {
				.size = 8,
				.banks = {
						  Bank{.addr = 0x0000, .bank = U16(bank_map[2]), .size = bank_1k},
						  Bank{.addr = 0x0400, .bank = U16(bank_map[3]), .size = bank_1k},
						  Bank{.addr = 0x0800, .bank = U16(bank_map[4]), .size = bank_1k},
						  Bank{.addr = 0x0C00, .bank = U16(bank_map[5]), .size = bank_1k},
						  Bank{.addr = 0x1000, .bank = U16(bank_map[0] + 0), .size = bank_1k},
						  Bank{.addr = 0x1400, .bank = U16(bank_map[0] + 1), .size = bank_1k},
						  Bank{.addr = 0x1800, .bank = U16(bank_map[1] + 0), .size = bank_1k},
						  Bank{.addr = 0x1C00, .bank = U16(bank_map[1] + 1), .size = bank_1k},
						  }
            };
		}
	}

	MirroringMode NesMapper004::mirroring() const noexcept
	{
		using enum MirroringMode;

		auto hw_mode = mirroring_mode(rom());

		switch (hw_mode)
		{
		case one_screen:
		case four_screen:
			return hw_mode;

		default:
			// runtime configured H or V
			// bit-0 controls mirroring with 1 == H and 0 == V
			// this is inverted from the iNES format, so invert the bit first
			return MirroringMode((~mirror) & 1);
		}
	}

	size_t NesMapper004::map_addr_cpu(U16 addr) noexcept
	{
		if (addr < 0x8000)
			LOG_CRITICAL("Address is out of range!");

		// 0 or 1
		int mode = (bank_select >> 6) & 1;

		auto num_banks = prgrom_banks(rom(), bank_8k);

		// everything from E000 - FFFF uses the last bank.
		auto bank = num_banks - 1;

		if (addr < 0xA000)
			bank = mode == 0 ? bank_map[6] : num_banks - 2;

		else if (addr < 0xC000)
			bank = bank_map[7];

		else if (addr < 0xE000)
			bank = mode == 0 ? num_banks - 2 : bank_map[6];

		return static_cast<U16>(bank * bank_8k + (addr & (bank_8k - 1)));
	}

	size_t NesMapper004::map_addr_ppu(U16 addr) noexcept
	{
		if (addr >= 0x2000)
			LOG_CRITICAL("Address is out of range!");

		auto mode = (bank_select >> 7) & 1;
		size_t bank = 0;

		if (addr < 0x0400)
			bank = mode == 0 ? bank_map[0] : bank_map[2];
		else if (addr < 0x0800)
			bank = mode == 0 ? bank_map[0] + 1 : bank_map[3];
		else if (addr < 0x0C00)
			bank = mode == 0 ? bank_map[1] : bank_map[4];
		else if (addr < 0x1000)
			bank = mode == 0 ? bank_map[1] + 1 : bank_map[5];
		else if (addr < 0x1400)
			bank = mode == 0 ? bank_map[2] : bank_map[0];
		else if (addr < 0x1800)
			bank = mode == 0 ? bank_map[3] : bank_map[0] + 1;
		else if (addr < 0x1c00)
			bank = mode == 0 ? bank_map[4] : bank_map[1];
		else if (addr < 0x2000)
			bank = mode == 0 ? bank_map[5] : bank_map[1] + 1;

		return static_cast<U16>(bank_1k * bank + (addr & (bank_1k - 1)));
	}

	void NesMapper004::update_irq(U16 addr) noexcept
	{
		// counter decremented on the rising edge of address line 12
		auto old_a12 = std::exchange(a12, (addr >> 12) & 1);

		if (old_a12 == 0 && a12 == 1 && (nes->ppu().current_tick() - cycle_low) > 10)
		{
			if (irq_counter == 0 || irq_reload)
			{
				irq_counter = irq_latch;
				irq_reload = false;
			}
			else
				--irq_counter;

			if (irq_counter == 0 && irq_enabled)
				signal_irq(true);
		}
		else if (old_a12 == 1 && a12 == 0)
		{
			cycle_low = nes->ppu().current_tick();
		}
	}

	U8 NesMapper004::on_cpu_read(U16 addr) noexcept
	{
		if (addr < 0x6000)
		{
			LOG_ERROR("Read to invalid address ${:04X}, ignoring", addr);
			return 0;
		}

		// prg-ram
		if (addr < 0x8000)
			return prg_ram[addr & (bank_8k - 1)];

		// address >= 8000
		return rom().prg_rom[map_addr_cpu(addr)];
	}

	void NesMapper004::on_cpu_write(U16 addr, U8 value) noexcept
	{
		if (addr < 0x6000)
		{
			LOG_ERROR("Write to invalid address ${:04X}, ignoring", addr);
			return;
		}

		// prg-ram
		if (addr < 0x8000)
		{
			prg_ram[addr & (bank_8k - 1)] = value;
			return;
		}

		// mask off the unimportant bits of the address so we can switch to the correct register
		// each 4k region has an even and odd register
		U16 reg = addr & 0b11100000'00000001;

		switch (reg)
		{
		default:
			LOG_CRITICAL("Unexpected write to reg {:04X} addr {:04X} with value {}, switch should be exhaustive", reg, addr, value);
			break;

		case 0x8000:
			bank_select = value;
			break;

		case 0x8001:
		{
			auto bank = value;
			auto index = bank_select & 7;

			U8 bank_mask;

			// banks at 2-5 are 1K chr-rom banks
			if (index < 6)
				bank_mask = U8(chr_banks(rom(), bank_1k) - 1);

			// banks 6-7 are prg-rom banks. According to the NESdev wiki, the MMC3 only has 6 prg-rom address lines, so the high bits are ignored. Some romhacks use all 8.
			else if (index < 8)
				bank_mask = U8(prgrom_banks(rom(), bank_8k) - 1);

			// banks at 0 and 1 are 2K chr-rom banks. Only even numbered banks are allowed, and the hardware ignores the low bit
			if (index < 2)
				bank_mask &= 0xFE;

			bank_map[index] = bank & bank_mask;
			break;
		}
		case 0xA000:
			mirror = value;
			break;

		case 0xA001:
			prg_ram_protect = value;
			break;

		case 0xC000:
			irq_latch = value;
			break;

		case 0xC001:
			// irq reload
			irq_reload = true;
			break;

		case 0xE000:
			// this disables interrupts and acknowledges any pending interrupts... I guess a game has to disable and then enable interrupts to acknowledge them?
			signal_irq(false);
			irq_enabled = false;
			break;

		case 0xE001:
			irq_enabled = true;
			break;
		}
	}

	std::optional<U8> NesMapper004::on_ppu_read(U16 &addr) noexcept
	{
		if (addr < 0x2000)
		{
			update_irq(addr);
			return chr_read(map_addr_ppu(addr));
		}
		// reading from the nametable
		else if (addr < 0x3F00)
		{
			apply_hardware_nametable_mapping(mirroring(), addr);
		}

		return std::nullopt;
	}

	bool NesMapper004::on_ppu_write(U16 &addr, U8 value) noexcept
	{
		if (addr < 0x2000)
		{
			update_irq(addr);
			return chr_write(map_addr_ppu(addr), value);
		}
		else if (addr < 0x3F00)
		{
			apply_hardware_nametable_mapping(mirroring(), addr);
		}
		return false;
	}
}
