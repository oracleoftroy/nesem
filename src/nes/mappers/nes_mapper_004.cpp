#include "nes_mapper_004.hpp"

#include <utility>

#include "nes.hpp"

#include <util/logging.hpp>

namespace nesem::mappers
{
	namespace
	{
		// used for MMC6 variant
		constexpr U8 prg_ram_protect_read_lo = 0b0010'0000;
		constexpr U8 prg_ram_protect_read_hi = 0b1000'0000;
		constexpr U8 prg_ram_protect_read = prg_ram_protect_read_lo | prg_ram_protect_read_hi;

		// note that to write, you also have to be able to read, thus both read and write bits appear in the mask
		constexpr U8 prg_ram_protect_write_lo = 0b0011'0000;
		constexpr U8 prg_ram_protect_write_hi = 0b1100'0000;

		constexpr U8 prg_ram_write_protect = 0b0100'0000;
		constexpr U8 prg_ram_enable = 0b1000'0000;

		NesMapper004Variants pick_variant(const NesRom &rom) noexcept
		{
			using enum NesMapper004Variants;
			if (rom.v2)
			{
				auto variant = NesMapper004Variants(rom.v2->pcb.submapper);

				// make sure the submapper is one of our enum values
				switch (variant)
				{
				case MMC3C:
					return variant;
				case MMC6:
					return variant;
				case MC_ACC:
					return variant;
				case MMC3A:
					return variant;
				}
			}

			// use MMC3C as the default submapper. This could trigger if the ROM is assigned to the deprecated submapper 2 or otherwise has a bad value
			return MMC3C;
		}
	}

	NesMapper004::NesMapper004(const Nes &nes, NesRom &&rom_data) noexcept
		: NesCartridge(nes, std::move(rom_data)), variant(pick_variant(rom()))
	{
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

		a12 = true;

		m2_state = NesBusOp::pending;
		m2_toggle_count = 0;
	}

	Banks NesMapper004::report_cpu_mapping() const noexcept
	{
		int mode = (bank_select >> 6) & 1;
		auto num_banks = rom_prgrom_banks(rom(), bank_8k);

		U16 bank0 = bank_map[6];
		U16 bank1 = bank_map[7];
		U16 bank2 = U16(num_banks - 2);
		U16 bank3 = U16(num_banks - 1);

		if (mode == 1)
			std::swap(bank0, bank2);

		return {
			.size = 4,
			.banks{
				Bank{.addr = 0x8000, .bank = bank0, .size = bank_8k},
				Bank{.addr = 0xA000, .bank = bank1, .size = bank_8k},
				Bank{.addr = 0xC000, .bank = bank2, .size = bank_8k},
				Bank{.addr = 0xE000, .bank = bank3, .size = bank_8k},
			},
		};
	}

	Banks NesMapper004::report_ppu_mapping() const noexcept
	{
		auto mode = (bank_select >> 7) & 1;

		if (mode == 0)
		{
			return {
				.size = 8,
				.banks{
					Bank{.addr = 0x0000, .bank = U16(bank_map[0] + 0), .size = bank_1k},
					Bank{.addr = 0x0400, .bank = U16(bank_map[0] + 1), .size = bank_1k},
					Bank{.addr = 0x0800, .bank = U16(bank_map[1] + 0), .size = bank_1k},
					Bank{.addr = 0x0C00, .bank = U16(bank_map[1] + 1), .size = bank_1k},
					Bank{.addr = 0x1000, .bank = U16(bank_map[2]), .size = bank_1k},
					Bank{.addr = 0x1400, .bank = U16(bank_map[3]), .size = bank_1k},
					Bank{.addr = 0x1800, .bank = U16(bank_map[4]), .size = bank_1k},
					Bank{.addr = 0x1C00, .bank = U16(bank_map[5]), .size = bank_1k},
				},
			};
		}
		else
		{
			return {
				.size = 8,
				.banks{
					Bank{.addr = 0x0000, .bank = U16(bank_map[2]), .size = bank_1k},
					Bank{.addr = 0x0400, .bank = U16(bank_map[3]), .size = bank_1k},
					Bank{.addr = 0x0800, .bank = U16(bank_map[4]), .size = bank_1k},
					Bank{.addr = 0x0C00, .bank = U16(bank_map[5]), .size = bank_1k},
					Bank{.addr = 0x1000, .bank = U16(bank_map[0] + 0), .size = bank_1k},
					Bank{.addr = 0x1400, .bank = U16(bank_map[0] + 1), .size = bank_1k},
					Bank{.addr = 0x1800, .bank = U16(bank_map[1] + 0), .size = bank_1k},
					Bank{.addr = 0x1C00, .bank = U16(bank_map[1] + 1), .size = bank_1k},
				},
			};
		}
	}

	MirroringMode NesMapper004::mirroring() const noexcept
	{
		using enum MirroringMode;

		auto hw_mode = rom_mirroring_mode(rom());

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

	size_t NesMapper004::map_addr_cpu(Addr addr) const noexcept
	{
		if (addr < 0x8000)
			LOG_CRITICAL("Address is out of range!");

		// 0 or 1
		auto mode = (bank_select >> 6) & 1;

		auto num_banks = rom_prgrom_banks(rom(), bank_8k);

		// everything from E000 - FFFF uses the last bank.
		auto bank = num_banks - 1;

		if (addr < 0xA000)
			bank = mode == 0 ? bank_map[6] : num_banks - 2;

		else if (addr < 0xC000)
			bank = bank_map[7];

		else if (addr < 0xE000)
			bank = mode == 0 ? num_banks - 2 : bank_map[6];

		return to_rom_addr(bank, bank_8k, addr);
	}

	size_t NesMapper004::map_addr_ppu(Addr addr) const noexcept
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

		return to_rom_addr(bank, bank_1k, addr);
	}

	void NesMapper004::signal_m2(bool rising) noexcept
	{
		if (rising)
			return;

		// NesDev - https://www.nesdev.org/wiki/MMC3
		// The MMC3 scanline counter is based entirely on PPU A12, triggered on a rising edge after the line has remained low for three falling edges of M2.

		// As I understand it, there are three S/R latches with a12 set to the reset and m2 the clock
		// m2 is a cpu output pin that goes low at the start of each cycle
		if (!a12)
			++m2_toggle_count;
		else
			m2_toggle_count = 0;
	}

	void NesMapper004::update_a12(Addr addr) noexcept
	{
		// counter decremented on the rising edge of address line 12
		auto old_a12 = std::exchange(a12, (addr & (1 << 12)) > 0);

		// NesDev - https://www.nesdev.org/wiki/MMC3
		// The MMC3 scanline counter is based entirely on PPU A12, triggered on a rising edge after the line has remained low for three falling edges of M2.
		if (!old_a12 && a12 && m2_toggle_count >= 3)
		{
			// record the previous count and whether this is a force reload so we can handle board variants later on
			auto prev_count = irq_counter;
			auto was_reload = irq_reload;

			if (irq_counter == 0 || irq_reload)
			{
				irq_counter = irq_latch;
				irq_reload = false;
			}
			else
				--irq_counter;

			if (irq_counter == 0 && irq_enabled)
			{
				// signal if:
				// all variants - the counter being 0 is enough to trigger an irq
				// MMC3A - only trigger if this was force reloaded to 0 or if we decremented to 0. If the counter was already 0 and reloaded to 0, do not trigger
				if (variant != NesMapper004Variants::MMC3A || was_reload || prev_count != irq_counter)
					signal_irq(true);
			}
		}
	}

	U8 NesMapper004::do_read_ram(size_t addr) const noexcept
	{
		// Mapper 004 carts in nescartdb either have ram or nvram (or neither)
		if (prgnvram_size() > 0)
			return prgnvram_read(addr);

		if (prgram_size() > 0)
			return prgram_read(addr);

		return open_bus_read();
	}

	bool NesMapper004::do_read_write(size_t addr, U8 value) noexcept
	{
		// Mapper 004 carts in nescartdb either have ram or nvram (or neither)
		if (prgnvram_size() > 0)
			return prgnvram_write(addr, value);

		if (prgram_size() > 0)
			return prgram_write(addr, value);

		return false;
	}

	U8 NesMapper004::on_cpu_peek(Addr addr) const noexcept
	{
		if (addr < 0x6000)
			return open_bus_read();

		// prg-ram
		if (addr < 0x8000)
		{
			if (prgram_size() == 0 && prgnvram_size() == 0)
				return open_bus_read();

			else if (variant == NesMapper004Variants::MMC6)
			{
				if (addr < 0x7000)
					return open_bus_read();

				// The PRG RAM protect bits control mapping of two 512B banks of RAM mirrored across $7000-7FFF.
				// If neither bank is enabled for reading, the $7000-$7FFF area is open bus.
				if ((prg_ram_protect & prg_ram_protect_read) == 0)
					return open_bus_read();

				// If only one bank is enabled for reading, the other reads back as zero.
				auto enabled_bit = (addr & 512) != 0 ? prg_ram_protect_read_hi : prg_ram_protect_read_lo;

				if (prg_ram_protect & enabled_bit)
					return do_read_ram(to_rom_addr(0, bank_1k, addr));
				else
					return 0;
			}
			else
			{
				if (prg_ram_protect & prg_ram_enable)
					return do_read_ram(to_rom_addr(0, bank_8k, addr));
				else
					return open_bus_read();
			}
		}

		// address >= 8000
		return rom().prg_rom[map_addr_cpu(addr)];
	}

	void NesMapper004::on_cpu_write(Addr addr, U8 value) noexcept
	{
		if (addr < 0x6000)
			return;

		// prg-ram
		if (addr < 0x8000)
		{
			// no ram, so nothing to do
			if (prgram_size() == 0 && prgnvram_size() == 0)
				return;

			if (variant == NesMapper004Variants::MMC6)
			{
				auto enabled_bits = (addr & 512) != 0 ? prg_ram_protect_write_hi : prg_ram_protect_write_lo;

				// The write-enable bits only have effect if that bank is enabled for reading, otherwise the bank is not writable.
				if ((prg_ram_protect & enabled_bits) == enabled_bits)
					do_read_write(to_rom_addr(0, bank_1k, addr), value);
			}
			else
			{
				if ((prg_ram_protect & prg_ram_write_protect) == 0)
					do_read_write(to_rom_addr(0, bank_8k, addr), value);
			}

			return;
		}

		// mask off the unimportant bits of the address so we can switch to the correct register
		// each 4k region has an even and odd register
		U16 reg = to_integer(addr & 0b11100000'00000001);

		switch (reg)
		{
		default:
			LOG_CRITICAL("Unexpected write to reg {:04X} addr {} with value {}, switch should be exhaustive", reg, addr, value);
			break;

		case 0x8000:
			bank_select = value;
			break;

		case 0x8001:
		{
			auto bank = value;
			auto index = bank_select & 7;

			U8 bank_mask = 0;

			// banks at 2-5 are 1K chr-rom banks
			if (index < 6)
				bank_mask = U8(rom_chr_banks(rom(), bank_1k) - 1);

			// banks 6-7 are prg-rom banks. According to the NESdev wiki, the MMC3 only has 6 prg-rom address lines, so the high bits are ignored. Some romhacks use all 8.
			else if (index < 8)
				bank_mask = U8(rom_prgrom_banks(rom(), bank_8k) - 1);

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

		// When PRG RAM is disabled via $8000, the mapper continuously sets $A001 to $00, and so all writes to $A001 are ignored.
		if (variant == NesMapper004Variants::MMC6 && (bank_select & (1 << 5)) == 0)
			prg_ram_protect = 0;
	}

	std::optional<U8> NesMapper004::on_ppu_peek(Addr &addr) const noexcept
	{
		if (addr < 0x2000)
			return chr_read(map_addr_ppu(addr));

		// reading from the nametable
		else if (addr < 0x3F00)
			apply_hardware_nametable_mapping(mirroring(), addr);

		return std::nullopt;
	}

	std::optional<U8> NesMapper004::on_ppu_read(Addr &addr) noexcept
	{
		update_a12(addr);

		return on_ppu_peek(addr);
	}

	bool NesMapper004::on_ppu_write(Addr &addr, U8 value) noexcept
	{
		update_a12(addr);

		if (addr < 0x2000)
			return chr_write(map_addr_ppu(addr), value);

		else if (addr < 0x3F00)
			apply_hardware_nametable_mapping(mirroring(), addr);

		return false;
	}
}
