#include "nes_mapper_005.hpp"

#include "../nes_ppu_register_bits.hpp"

namespace nesem::mappers
{
	NesMapper005::NesMapper005(const Nes &nes, NesRom &&rom) noexcept
		: NesCartridge(nes, std::move(rom))
	{
		reset();
	}

	void NesMapper005::reset() noexcept
	{
		ppu_state = PpuStateMirror::none;
		prg_mode = 0xFF;
		chr_mode = 0xFF;
		prg_ram_protect = 0xFF;
		internal_ram_mode = 0xFF;
		nametable_mapping = 0xFF;
		fill_mode_tile = 0xFF;
		fill_mode_color = 0xFF;
		nametable_fill = 0xFF;

		prg_banks = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
		chr_banks = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

		vertical_split_mode = 0;
		vertical_split_scroll = 0;
		vertical_split_bank = 0;

		scanline_irq_compare = 0;
		scanline_irq_enabled = false;
		current_scanline = 0;
		mul_a = 0;
		mul_b = 0;
		mul_ans = 0;
	}

	Banks NesMapper005::report_cpu_mapping() const noexcept
	{
		// TODO: implement me
		return Banks{};
	}

	Banks NesMapper005::report_ppu_mapping() const noexcept
	{
		// TODO: implement me
		return Banks{};
	}

	MirroringMode NesMapper005::mirroring() const noexcept
	{
		// TODO: implement me
		return MirroringMode::horizontal;
	}

	size_t NesMapper005::map_addr_cpu(Addr addr) const noexcept
	{
		// PRG mode 0
		// CPU $6000-$7FFF: 8 KB switchable PRG RAM bank
		// CPU $8000-$FFFF: 32 KB switchable PRG ROM bank
		// PRG mode 1
		// CPU $6000-$7FFF: 8 KB switchable PRG RAM bank
		// CPU $8000-$BFFF: 16 KB switchable PRG ROM/RAM bank
		// CPU $C000-$FFFF: 16 KB switchable PRG ROM bank
		// PRG mode 2
		// CPU $6000-$7FFF: 8 KB switchable PRG RAM bank
		// CPU $8000-$BFFF: 16 KB switchable PRG ROM/RAM bank
		// CPU $C000-$DFFF: 8 KB switchable PRG ROM/RAM bank
		// CPU $E000-$FFFF: 8 KB switchable PRG ROM bank
		// PRG mode 3
		// CPU $6000-$7FFF: 8 KB switchable PRG RAM bank
		// CPU $8000-$9FFF: 8 KB switchable PRG ROM/RAM bank
		// CPU $A000-$BFFF: 8 KB switchable PRG ROM/RAM bank
		// CPU $C000-$DFFF: 8 KB switchable PRG ROM/RAM bank
		// CPU $E000-$FFFF: 8 KB switchable PRG ROM bank

		// TODO: implement me
		return 0;
	}

	size_t NesMapper005::map_addr_ppu(Addr addr) const noexcept
	{
		// CHR mode 0
		// PPU $0000-$1FFF: 8 KB switchable CHR bank
		// CHR mode 1
		// PPU $0000-$0FFF: 4 KB switchable CHR bank
		// PPU $1000-$1FFF: 4 KB switchable CHR bank
		// CHR mode 2
		// PPU $0000-$07FF: 2 KB switchable CHR bank
		// PPU $0800-$0FFF: 2 KB switchable CHR bank
		// PPU $1000-$17FF: 2 KB switchable CHR bank
		// PPU $1800-$1FFF: 2 KB switchable CHR bank
		// CHR mode 3
		// PPU $0000-$03FF: 1 KB switchable CHR bank
		// PPU $0400-$07FF: 1 KB switchable CHR bank
		// PPU $0800-$0BFF: 1 KB switchable CHR bank
		// PPU $0C00-$0FFF: 1 KB switchable CHR bank
		// PPU $1000-$13FF: 1 KB switchable CHR bank
		// PPU $1400-$17FF: 1 KB switchable CHR bank
		// PPU $1800-$1BFF: 1 KB switchable CHR bank
		// PPU $1C00-$1FFF: 1 KB switchable CHR bank

		// TODO: implement me
		return 0;
	}

	U8 NesMapper005::on_cpu_peek(Addr addr) const noexcept
	{
		if (addr == 0x5204)
		{
			bool in_frame = current_scanline == scanline_irq_compare;
			return U8((scanline_irq_enabled << 7) | (in_frame << 6));
		}

		else if (addr == 0x5205)
			return U8(mul_ans & 0xFF);

		// multiplier 2
		else if (addr == 0x5206)
			return U8((mul_ans >> 8) & 0xFF);

		// TODO: memory map / read

		return open_bus_read();
	}

	void NesMapper005::on_cpu_write(Addr addr, U8 value) noexcept
	{
		// Observe PPU register stuff
		// NesWiki indicates that the MMC5 observes other PPU state, but with no known effect.

		// PPUCTRL
		if (addr == 0x2000)
		{
			ppu_state.set(value & ctrl_sprite_8x16, PpuStateMirror::sprite8x16);
		}

		// PPUMASK
		else if (addr == 0x2001)
		{
			ppu_state.set(value & mask_show_sprites, PpuStateMirror::show_sprites);
			ppu_state.set(value & mask_show_background, PpuStateMirror::show_background);
		}

		// sound (not implemented)
		else if (addr >= 0x5000 && addr < 0x5016)
		{
			return;
		}

		// PRG mode
		else if (addr == 0x5100)
			prg_mode = 0x03 & value;

		// CHR mode
		else if (addr == 0x5101)
			chr_mode = 0x03 & value;

		// PRG RAM protect 1
		else if (addr == 0x5102)
			prg_ram_protect = (prg_ram_protect & 0b0011) | (0x03 & value);

		// PRG RAM protect 2
		else if (addr == 0x5103)
			prg_ram_protect = (prg_ram_protect & 0b1100) | U8((0x03 & value) << 2);

		// internal extended RAM mode
		else if (addr == 0x5104)
			internal_ram_mode = 0x03 & value;

		// nametable mapping
		else if (addr == 0x5105)
			nametable_mapping = value;

		// fill mode tile
		else if (addr == 0x5106)
			fill_mode_tile = value;

		// fill mode color
		else if (addr == 0x5107)
			fill_mode_color = 0x03 & value;

		// PRG bankswitching
		else if (addr >= 0x5113 && addr < 0x5118)
			prg_banks[to_integer(addr - 0x5113)] = value;

		// CHR bankswitching
		else if (addr >= 0x5120 && addr < 0x512c)
			chr_banks[to_integer(addr - 0x5120)] = value;

		// Vertical split mode
		else if (addr == 0x5200)
			vertical_split_mode = value;

		// Vertical split scroll
		else if (addr == 0x5201)
			vertical_split_scroll = value;

		// Vertical split bank
		else if (addr == 0x5202)
			vertical_split_bank = value;

		// scanline IRQ compare value
		else if (addr == 0x5203)
			scanline_irq_compare = value;

		// scanline IRQ status
		else if (addr == 0x5204)
			scanline_irq_enabled = (value & 0b1000'0000) > 0;

		// multiplier 1
		else if (addr == 0x5205)
		{
			mul_a = value;
			mul_ans = mul_a * mul_b;
		}

		// multiplier 2
		else if (addr == 0x5206)
		{
			mul_b = value;
			mul_ans = mul_a * mul_b;
		}

		// Registers $5207, $5208, $5209, $520A, and range $5800-$5BFF are present only in MMC5A.

		else if (addr == 0x5207)
		{
			// CL3 / SL3 Data Direction and Output Data Source (MMC5A: $5207 write only)
		}

		else if (addr == 0x5208)
		{
			// CL3 / SL3 Status (MMC5A: $5208 read/write)
		}

		else if (addr == 0x5209)
		{
			// 16-bit Hardware Timer with IRQ (MMC5A: $5209 read/write, $520A write)
			//  timer LSB
		}

		else if (addr == 0x520A)
		{
			// timer MSB
		}
	}

	std::optional<U8> NesMapper005::on_ppu_peek(Addr &addr) const noexcept
	{
		// TODO: implement me
		return std::nullopt;
	}

	std::optional<U8> NesMapper005::on_ppu_read(Addr &addr) noexcept
	{
		// TODO: implement me
		return std::nullopt;
	}

	bool NesMapper005::on_ppu_write(Addr &addr, U8 value) noexcept
	{
		// TODO: implement me
		return false;
	}

}
