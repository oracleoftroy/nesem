#include "nes_mapper_004.hpp"

#include <utility>

#include <util/logging.hpp>

namespace nesem::mappers
{
	NesMapper004::NesMapper004(NesRom &&rom) noexcept
		: NesCartridge(std::move(rom))
	{
		prg_ram.resize(bank_8k);
	}

	void NesMapper004::reset() noexcept
	{
	}

	size_t NesMapper004::map_addr_cpu(U16 addr) noexcept
	{
		if (addr < 0x8000)
			LOG_CRITICAL("Address is out of range!");

		// 0 or 1
		int mode = (bank_select >> 6) & 1;

		auto num_banks = prgrom_banks(rom, bank_8k);

		// everything from E000 - FFFF uses the last bank.
		auto bank = num_banks - 1;

		if (addr < 0xA000)
			bank = mode == 0 ? bank_map[6] : num_banks - 2;

		else if (addr < 0xC000)
			bank = bank_map[7];

		else if (addr < 0xE000)
			bank = mode == 0 ? num_banks - 2 : bank_map[6];

		bank %= num_banks;

		return static_cast<U16>(bank * bank_8k + (addr & (bank_8k - 1)));
	}

	size_t NesMapper004::map_addr_ppu(U16 addr) noexcept
	{
		if (addr >= 0x2000)
			LOG_CRITICAL("Address is out of range!");

		auto mode = (bank_select >> 7) & 1;
		auto num_banks = chr_banks(rom, bank_1k);
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

		bank %= num_banks;

		return static_cast<U16>(bank_1k * bank + (addr & (bank_1k - 1)));
	}

	void NesMapper004::update_irq(U16 addr) noexcept
	{
		// counter decremented on the rising edge of address line 12
		auto current_a12 = (addr >> 12) & 1;

		if (a12 == 0 && current_a12 == 1)
		{
			if (irq_counter == 0 || irq_reload)
				irq_counter = irq_latch;
			else
				--irq_counter;

			if (irq_counter == 0 && irq_enabled)
				irq_signaled = true;
		}
	}

	U8 NesMapper004::cpu_read(U16 addr) noexcept
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
		return rom.prg_rom[map_addr_cpu(addr)];
	}

	void NesMapper004::cpu_write(U16 addr, U8 value) noexcept
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

		// writes to
		U16 reg = ((addr >> 12) & 0b110) | (addr & 1);

		switch (reg)
		{
		default:
			LOG_CRITICAL("Unexpected write to {:04X} with value {}, switch should be exhaustive", addr, value);
			break;

		case 0:
			bank_select = value;
			break;

		case 1:
		{
			auto bank = value;
			auto index = bank_select & 7;

			// banks at 0 and 1 are 2K chr-rom banks. Only even numbered banks are allowed, and the hardware ignores the low bit
			if (index < 2)
				bank &= 0xFE;

			// banks at 2-5 are 1K chr-rom banks, nothing to do

			// banks 6-7 are prg-rom banks. According to the NESdev wiki, the MMC3 only has 6 prg-rom address lines, so the high bits are ignored. Some romhacks use all 8.
			else if (index < 8)
				bank &= 0b00111111;

			bank_map[index] = bank;
			break;
		}
		case 2:
			mirroring = value;
			break;

		case 3:
			prg_ram_protect = value;
			break;

		case 4:
			irq_latch = value;
			break;

		case 5:
			// irq reload
			irq_reload = true;
			break;

		case 6:
			// this disables interrupts and acknowledges any pending interrupts... I guess a game has to disable and then enable interrupts to acknowledge them?
			irq_signaled = false;
			irq_enabled = false;
			break;

		case 7:
			irq_enabled = true;
			break;
		}
	}

	std::optional<U8> NesMapper004::ppu_read(U16 &addr) noexcept
	{
		update_irq(addr);

		if (addr < 0x2000)
			return rom.chr_rom[map_addr_ppu(addr)];

		// reading from the nametable
		else if (addr < 0x3F00)
		{
			if (auto mode = mirroring_mode(rom);
				mode == ines_2::MirroringMode::one_screen || mode == ines_2::MirroringMode::four_screen)
				apply_hardware_nametable_mapping(rom, addr);
			else
			{
				// if currently configured for horizontal mapping
				if ((mirroring & 1) == 1)
				{
					addr = (addr & ~0b0'000'11'00000'00000) |
						((addr & 0b0'000'10'00000'00000) >> 1) |
						((addr & 0b0'000'01'00000'00000) << 1);
				}
			}
		}

		return std::nullopt;
	}

	bool NesMapper004::ppu_write(U16 &addr, U8 value) noexcept
	{
		update_irq(addr);

		if (addr < 0x2000)
		{
			if (has_chrram(rom))
			{
				rom.chr_rom[map_addr_ppu(addr)] = value;
				return true;
			}

			// This shouldn't happen....
			LOG_WARN("PPU write to CHR-ROM, ignoring");
		}
		else if (addr < 0x3F00)
		{
			if (auto mode = mirroring_mode(rom);
				mode == ines_2::MirroringMode::one_screen || mode == ines_2::MirroringMode::four_screen)
				apply_hardware_nametable_mapping(rom, addr);
			else
			{
				// if currently configured for horizontal mapping
				if ((mirroring & 1) == 1)
				{
					addr = (addr & ~0b0'000'11'00000'00000) |
						((addr & 0b0'000'10'00000'00000) >> 1) |
						((addr & 0b0'000'01'00000'00000) << 1);
				}
			}
		}
		return false;
	}
}
