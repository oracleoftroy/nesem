#include "nes_mapper_001.hpp"

#include <algorithm>

#include "nes.hpp"

#include <util/logging.hpp>

namespace nesem::mappers
{
	NesMapper001::NesMapper001(const Nes &nes, NesRom &&rom_data) noexcept
		: NesCartridge(nes, std::move(rom_data))
	{
		// the amount of chr data is a multiple of 8k
		// this calculates a mask for the significant bits of chr_bank_x for a 4k bank size
		//   8k >> 12 - 1 == 0b00001
		//  16k >> 12 - 1 == 0b00011
		//  32k >> 12 - 1 == 0b00111
		//  64k >> 12 - 1 == 0b01111
		// 128k >> 12 - 1 == 0b11111
		chr_bank_mask = U8((chr_size() >> 12) - 1);
		switch (chr_bank_mask)
		{
		case 0b00001:
		case 0b00011:
		case 0b00111:
		case 0b01111:
		case 0b11111:
			// all valid masks
			break;

		default:
			LOG_CRITICAL("Invalid CHR-ROM mask!");
			break;
		}

		// SZROM has 8K of PRG RAM, 8K of PRG NV RAM, and 16K or more of CHR.
		if (rom().v2 &&
			rom().v2->prgram.value_or(0) == bank_8k &&
			rom().v2->prgnvram.value_or(0) == bank_8k &&
			size(rom().chr_rom) >= bank_16k)
			prg_ram_mode = PrgRamMode::SZROM;

		reset();
	}

	void NesMapper001::reset() noexcept
	{
		load_counter = 0;
		load_shifter = 0;
		control |= 0x0C;
		chr_bank_0 = 0;
		chr_bank_1 = 0;
		prg_bank = 0;
		last_write_cycle = 0;
	}

	Banks NesMapper001::report_cpu_mapping() const noexcept
	{
		auto [bank, first_bank, last_bank] = calculate_banks();

		auto bank_mode = (control >> 2) & 3;

		switch (bank_mode)
		{
		default:
			LOG_CRITICAL("This should never happen!");
			return {};

		case 0:
		case 1:
			//	32k at $8000
			return Banks{
				.size = 1,
				.banks{Bank{.addr = 0x8000, .bank = U16(bank >> 1), .size = bank_32k}},
			};

			//  2: fix first bank at $8000 and switch 16 KB bank at $C000;
		case 2:
			return Banks{
				.size = 2,
				.banks{
					Bank{.addr = 0x8000, .bank = first_bank, .size = bank_16k},
					Bank{.addr = 0xC000, .bank = bank, .size = bank_16k},
				},
			};

			//  3: fix last bank at $C000 and switch 16 KB bank at $8000)
		case 3:
			return Banks{
				.size = 2,
				.banks{
					Bank{.addr = 0x8000, .bank = bank, .size = bank_16k},
					Bank{.addr = 0xC000, .bank = last_bank, .size = bank_16k},
				},
			};
		}
	}

	Banks NesMapper001::report_ppu_mapping() const noexcept
	{
		auto bank_mode = (control >> 4) & 1;

		if (bank_mode == 0)
		{
			auto bank = U16((chr_bank_0 & chr_bank_mask) >> 1);

			return Banks{
				.size = 1,
				.banks{Bank{.addr = 0x0000, .bank = bank, .size = bank_8k}},
			};
		}
		else
		{
			return Banks{
				.size = 2,
				.banks{
					Bank{.addr = 0x0000, .bank = U16(chr_bank_0 & chr_bank_mask), .size = bank_4k},
					Bank{.addr = 0x1000, .bank = U16(chr_bank_1 & chr_bank_mask), .size = bank_4k},
				},
			};
		}
	}

	U8 NesMapper001::on_cpu_peek(Addr addr) const noexcept
	{
		if (addr < 0x6000)
			return 0;

		if (addr < 0x8000)
		{
			auto ram_addr = map_prgram_addr(addr);

			// if we have both prgram and nvram, assume the battery backed ram is in the upper bank
			if (prgram_size() > 0 && prgnvram_size() > 0)
			{
				CHECK(prgram_size() == prgnvram_size(), "All examples in romdb have the same amount for both");

				ram_addr &= ram_addr & (prgram_size() - 1);

				// SZROM has an 8k bank of RAM and an 8k bank of battery backed RAM
				// direct addresses mapped to the high bank to nvram
				if (ram_addr > prgram_size())
					return prgnvram_read(ram_addr);
				else
					return prgram_read(ram_addr);
			}

			if (prgnvram_size() > 0)
				return prgnvram_read(ram_addr);

			if (prgram_size() > 0)
				return prgram_read(ram_addr);

			return open_bus_read();
		}

		return rom().prg_rom[map_prgrom_addr(addr)];
	}

	void NesMapper001::on_cpu_write(Addr addr, U8 value) noexcept
	{
		if (addr < 0x6000)
			return;

		if (addr < 0x8000)
		{
			auto ram_addr = map_prgram_addr(addr);

			// if we have both prgram and nvram, assume the battery backed ram is in the upper bank
			if (prgram_size() > 0 && prgnvram_size() > 0)
			{
				CHECK(prgram_size() == prgnvram_size(), "All examples in romdb have the same amount for both");

				ram_addr &= ram_addr & (prgram_size() - 1);

				// SZROM has an 8k bank of RAM and an 8k bank of battery backed RAM
				// direct addresses mapped to the high bank to nvram
				if (ram_addr > prgram_size())
					prgnvram_write(ram_addr, value);
				else
					prgram_write(ram_addr, value);
				return;
			}

			if (prgnvram_size() > 0)
				prgnvram_write(ram_addr, value);

			else if (prgram_size() > 0)
				prgram_write(ram_addr, value);

			return;
		}

		if (auto load = shift(value);
			load.has_value())
		{
			// we have the value we want to load, so store it
			switch (to_integer(addr & 0xE000))
			{
			default:
				LOG_CRITICAL("BUG, all address ranges above $8000 should be handled, but address ${} got here?", addr);
				break;

			case 0x8000: // address between [8000-A000)
				control = *load;
				break;

			case 0xA000: // address between [A000-C000)
				chr_bank_0 = *load;
				break;

			case 0xC000: // address between [C000-E000)
				chr_bank_1 = *load;
				break;

			case 0xE000: // address between [E000-10000)
				prg_bank = *load;
				break;
			}
		}
	}

	std::optional<U8> NesMapper001::on_ppu_peek(Addr &addr) const noexcept
	{
		if (addr < 0x2000)
			return chr_read(map_ppu_addr(addr));

		// reading from the nametable
		else if (addr < 0x3F00)
			apply_hardware_nametable_mapping(mirroring(), addr);

		return std::nullopt;
	}

	bool NesMapper001::on_ppu_write(Addr &addr, U8 value) noexcept
	{
		if (addr < 0x2000)
			return chr_write(map_ppu_addr(addr), value);

		else if (addr < 0x3F00)
			apply_hardware_nametable_mapping(mirroring(), addr);

		return false;
	}

	MirroringMode NesMapper001::mirroring() const noexcept
	{
		using enum MirroringMode;

		auto mode = control & 3;
		switch (mode)
		{
		case 0:
			return one_screen;
		case 1:
			return four_screen;
		case 2:
			return vertical;
		case 3:
			return horizontal;
		}

		CHECK(false, "Should never get here");
		return horizontal;
	}

	std::optional<U8> NesMapper001::shift(U8 value) noexcept
	{
		std::optional<U8> result = std::nullopt;

		// reset the shifter if the high bit is set
		if (value & 0x80)
		{
			load_counter = 0;
			load_shifter = 0;
			control |= 0x0C;
		}
		else
		{
			auto prev = std::exchange(last_write_cycle, nes->cpu().current_cycle());
			if (prev + 1 < last_write_cycle)
			{
				load_shifter |= (value & 1) << load_counter;
				++load_counter;

				if (load_counter == 5)
				{
					result = std::exchange(load_shifter, U8(0));
					load_counter = 0;
				}
			}
		}

		return result;
	}

	auto NesMapper001::calculate_banks() const noexcept -> PrgRomBanks
	{
		U8 bank = prg_bank & 0b01111;
		U8 first_bank = 0;
		U8 last_bank_mask = 0b1111;

		// prg_bank can address up to 256k. A 512k cart stores an extra bit in chr_bank_0 and chr_bank_1.
		// NesDev wiki indicates that a game should always store the same value in both bits, so we'll
		// assume it is always safe to just use chr_bank_0

		// 512k prg-rom
		if (size(rom().prg_rom) == 0x80000)
		{
			U8 bank_ext = (chr_bank_0 & 0b10000);
			bank |= bank_ext;
			first_bank |= bank_ext;
			last_bank_mask |= bank_ext;
		}

		return PrgRomBanks{
			.bank = bank,
			.first_bank = first_bank,
			.last_bank = U8((rom_prgrom_banks(rom(), bank_16k) - 1) & last_bank_mask),
		};
	}

	size_t NesMapper001::map_prgram_addr(Addr addr) const noexcept
	{
		if (addr < 0x6000 || addr >= 0x8000) [[unlikely]]
		{
			LOG_CRITICAL("BUG, this should only be called with prg ram addresses, but was called with {}", addr);
			return 0;
		}

		size_t bank = 0;

		if (prg_ram_mode == PrgRamMode::SZROM)
			bank = (chr_bank_0 >> 4) & 1;
		else if (auto size = prgram_size() + prgnvram_size();
			size == bank_16k)
			bank = (chr_bank_0 >> 3) & 1;
		else if (size == bank_32k)
			bank = (chr_bank_0 >> 2) & 3;

		return to_rom_addr(bank, bank_8k, addr);
	}

	size_t NesMapper001::map_prgrom_addr(Addr addr) const noexcept
	{
		if (addr < 0x8000) [[unlikely]]
		{
			LOG_CRITICAL("BUG, this should only be called with prg rom addresses, but was called with {}", addr);
			return 0;
		}

		auto [bank, first_bank, last_bank] = calculate_banks();

		auto bank_mode = (control >> 2) & 3;

		switch (bank_mode)
		{
		default:
			LOG_CRITICAL("This should never happen!");
			return 0;

		case 0:
		case 1:
			//	32k at $8000
			bank >>= 1;
			return to_rom_addr(bank, bank_32k, addr);

			//  2: fix first bank at $8000 and switch 16 KB bank at $C000;
		case 2:
			if (addr < 0xC000)
				bank = first_bank;

			return to_rom_addr(bank, bank_16k, addr);

			//  3: fix last bank at $C000 and switch 16 KB bank at $8000)
		case 3:

			if (addr >= 0xC000)
				bank = last_bank;

			return to_rom_addr(bank, bank_16k, addr);
		}
	}

	size_t NesMapper001::map_ppu_addr(Addr addr) const noexcept
	{
		if (addr >= 0x2000) [[unlikely]]
		{
			LOG_CRITICAL("BUG, this should only be called with chr rom/ram addresses, but was called with {}", addr);
			return 0;
		}

		auto bank_mode = (control >> 4) & 1;

		if (bank_mode == 0)
		{
			auto bank = (chr_bank_0 & chr_bank_mask) >> 1;
			return to_rom_addr(bank, bank_8k, addr);
		}
		else
		{
			auto bank = addr >= 0x1000 ? chr_bank_1 : chr_bank_0;
			bank &= chr_bank_mask;

			return to_rom_addr(bank, bank_4k, addr);
		}
	}
}
