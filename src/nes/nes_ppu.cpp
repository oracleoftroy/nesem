#include "nes_ppu.hpp"

#include <util/logging.hpp>

#include "nes.hpp"

namespace nesem
{
	// clang-format off
	constexpr U16 vram_fine_y_mask    = 0b0'111'00'00000'00000;
	constexpr U16 vram_nametable_mask = 0b0'000'11'00000'00000;
	constexpr U16 vram_nametable_y_mask = 0b0'000'10'00000'00000;
	constexpr U16 vram_nametable_x_mask = 0b0'000'01'00000'00000;
	constexpr U16 vram_coarse_y_mask  = 0b0'000'00'11111'00000;
	constexpr U16 vram_coarse_x_mask  = 0b0'000'00'00000'11111;
	// clang-format on

	constexpr U16 vram_fine_y_shift = 12;
	constexpr U16 vram_nametable_shift = 10;
	constexpr U16 vram_coarse_y_shift = 5;
	constexpr U16 vram_coarse_x_shift = 0;

	// clang-format off
	// NOTE: first 5 bits of PPUSTATUS are unused
	constexpr U8 status_vblank          = 0b100'00000;
	constexpr U8 status_sprite0_hit     = 0b010'00000;
	constexpr U8 status_sprite_overflow = 0b001'00000;

	constexpr U8 ctrl_nmi_flag       = 0b1000'0000;
	constexpr U8 ctrl_master_flag    = 0b0100'0000;
	constexpr U8 ctrl_sprite_size    = 0b0010'0000;
	constexpr U8 ctrl_pattern_addr   = 0b0001'0000;
	constexpr U8 ctrl_sprite_addr    = 0b0000'1000;
	constexpr U8 ctrl_vram_addr_inc  = 0b0000'0100;
	constexpr U8 ctrl_nametable_mask = 0b0000'0011;

	constexpr U8 mask_emphasize_blue           = 0b1000'0000;
	constexpr U8 mask_emphasize_green          = 0b0100'0000;
	constexpr U8 mask_emphasize_red            = 0b0010'0000;
	constexpr U8 mask_show_sprites             = 0b0001'0000;
	constexpr U8 mask_show_background          = 0b0000'1000;
	constexpr U8 mask_show_leftmost_sprites    = 0b0000'0100;
	constexpr U8 mask_show_leftmost_background = 0b0000'0010;
	constexpr U8 mask_grayscale                = 0b0000'0001;
	// clang-format on

	NesPpu::NesPpu(Nes *nes) noexcept
		: nes(nes)
	{
		CHECK(nes != nullptr, "Nes is required");

		reset();
		palettes.fill(0);
	}

	void NesPpu::reset() noexcept
	{
		reg.ppuctrl = 0;
		reg.ppumask = 0;
		reg.ppustatus = 0;
		reg.oamaddr = 0;
		reg.addr_latch = false;
		reg.tram_addr = 0;
		reg.vram_addr = 0;

		reg.fine_x = 0;
		reg.ppudata = 0;

		frame = 0;
		cycle = 0;
		scanline = 0;
	}

	void NesPpu::load_cartridge(NesCartridge *cart) noexcept
	{
		this->cartridge = cart;
	}

	void NesPpu::draw_pattern_table(int index, U8 palette, const DrawFn &draw_pixel)
	{
		for (U16 tile_y = 0; tile_y < 16; ++tile_y)
		{
			for (U16 tile_x = 0; tile_x < 16; ++tile_x)
			{
				U16 offset = tile_y * 256 + tile_x * 16;

				for (U16 row = 0; row < 8; ++row)
				{
					U8 tile_lo = read(index * 0x1000 + offset + row + 0x0000);
					U8 tile_hi = read(index * 0x1000 + offset + row + 0x0008);

					for (U16 col = 0; col < 8; ++col)
					{
						U8 bit = 0x80 >> col;

						U16 palette_index = (palette << 2) | ((tile_hi & bit) != 0) << 1 | ((tile_lo & bit) != 0);

						draw_pixel(int(tile_x * 8 + col), int(tile_y * 8 + row), read(0x3F00 + palette_index));
					}
				}
			}
		}
	}

	void NesPpu::draw_name_table(int index, const DrawFn &draw_pixel)
	{
		U16 pattern_start = (reg.ppuctrl & ctrl_pattern_addr) != 0 ? 0x1000 : 0;

		for (U16 tile_y = 0; tile_y < 30; ++tile_y)
		{
			for (U16 tile_x = 0; tile_x < 32; ++tile_x)
			{
				U16 nt_addr = 0x2000 | ((index & 3) << vram_nametable_shift) | (tile_y << vram_coarse_y_shift) | tile_x;
				U16 attr_addr = 0x23C0 | ((index & 3) << vram_nametable_shift) | ((tile_y << 1) & 0b111000) | (tile_x >> 2);

				auto nt = read(nt_addr);
				auto attr = read(attr_addr);
				if (tile_y & 2)
					attr >>= 4;
				if (tile_x & 2)
					attr >>= 2;
				attr &= 3;

				// U16 offset = tile_y * 256 + tile_x * 16;

				for (U16 row = 0; row < 8; ++row)
				{
					U8 tile_lo = read(pattern_start | (nt << 4) | (row + 0x0000));
					U8 tile_hi = read(pattern_start | (nt << 4) | (row + 0x0008));

					for (U16 col = 0; col < 8; ++col)
					{
						U8 bit = 0x80 >> col;

						U16 palette_index = (attr << 2) | ((tile_hi & bit) != 0) << 1 | ((tile_lo & bit) != 0);

						draw_pixel(int(tile_x * 8 + col), int(tile_y * 8 + row), read(0x3F00 + palette_index));
					}
				}
			}
		}
	}

	void NesPpu::reload() noexcept
	{
		pattern_shifter_lo = (pattern_shifter_lo & 0xFF00) | next_pattern_lo;
		pattern_shifter_hi = (pattern_shifter_hi & 0xFF00) | next_pattern_hi;

		// handle the attribute data like the pattern so that fine_x works the same
		attribute_lo = U16((attribute_lo & 0xFF00) | ((next_attribute & 1) ? 0xFF : 0));
		attribute_hi = U16((attribute_hi & 0xFF00) | ((next_attribute & 2) ? 0xFF : 0));
	}

	void NesPpu::shift() noexcept
	{
		if (background_rendering_enabled())
		{
			pattern_shifter_lo <<= 1;
			pattern_shifter_hi <<= 1;

			attribute_lo <<= 1;
			attribute_hi <<= 1;
		}

		if (sprite_rendering_enabled())
		{
			// TODO: implement
		}
	}

	void NesPpu::increment_x() noexcept
	{
		if (rendering_enabled())
		{
			if ((reg.vram_addr & vram_coarse_x_mask) == 31)
			{
				// zero coarse_x and flip nt_x
				reg.vram_addr &= ~vram_coarse_x_mask;
				reg.vram_addr ^= vram_nametable_x_mask;
			}
			else
				++reg.vram_addr;
		}
	}

	void NesPpu::increment_y() noexcept
	{
		if (rendering_enabled())
		{
			auto fine_y = (reg.vram_addr & vram_fine_y_mask) >> vram_fine_y_shift;
			auto coarse_y = (reg.vram_addr & vram_coarse_y_mask) >> vram_coarse_y_shift;

			if (++fine_y > 7)
			{
				fine_y = 0;

				if (++coarse_y > 29)
				{
					coarse_y = 0;
					reg.vram_addr ^= vram_nametable_y_mask;
				}
			}

			reg.vram_addr = (reg.vram_addr & ~(vram_coarse_y_mask | vram_fine_y_mask)) | (coarse_y << vram_coarse_y_shift) | (fine_y << vram_fine_y_shift);
		}
	}

	void NesPpu::transfer_x() noexcept
	{
		if (rendering_enabled())
		{
			// transfer the x portions of tram to vram, leaving the rest as is
			reg.vram_addr = (reg.vram_addr & ~(vram_nametable_x_mask | vram_coarse_x_mask)) |
				(reg.tram_addr & vram_nametable_x_mask) |
				(reg.tram_addr & vram_coarse_x_mask);
		}
	}

	void NesPpu::transfer_y() noexcept
	{
		if (rendering_enabled())
		{
			// transfer the y portions of tram to vram, leaving the rest as is
			reg.vram_addr = (reg.vram_addr & ~(vram_nametable_y_mask | vram_coarse_y_mask | vram_fine_y_mask)) |
				(reg.tram_addr & vram_nametable_y_mask) |
				(reg.tram_addr & vram_coarse_y_mask) |
				(reg.tram_addr & vram_fine_y_mask);
		}
	}

	bool NesPpu::rendering_enabled() noexcept
	{
		return (reg.ppumask & (mask_show_background | mask_show_sprites)) != 0;
	}

	bool NesPpu::background_rendering_enabled() noexcept
	{
		return (reg.ppumask & mask_show_background) == mask_show_background;
	}

	bool NesPpu::sprite_rendering_enabled() noexcept
	{
		return (reg.ppumask & mask_show_sprites) == mask_show_sprites;
	}

	U16 NesPpu::make_chrrom_addr() noexcept
	{
		return ((reg.ppuctrl & ctrl_pattern_addr) ? 0x1000 : 0) | (tile_id << 4) | ((reg.vram_addr & vram_fine_y_mask) >> vram_fine_y_shift);
	}

	void NesPpu::clock() noexcept
	{
		// odd frame skip: the very first cycle of an odd frame is skipped if renering is enabled
		if (scanline == 0 && cycle == 0 && (frame & 1) == 1 && rendering_enabled())
			cycle = 1;

		// during these scanlines, we prepare data for rendering. Scanline 261 (or -1) prefetches data rendered during the first scanline
		if (scanline < 240 || scanline == 261)
		{
			if ((cycle >= 1 && cycle <= 257) || (cycle >= 321 && cycle <= 336))
			{
				if (cycle > 1)
					shift();

				switch ((cycle - 1) % 8)
				{
				case 0:
					if (cycle >= 8)
					{
						// we don't reload on cycle 1; the prior scanline already set up the data
						reload();
					}

					// load the tile id from the nametable currently addressed at vram_addr. Note the masks takes all but the fine_y position
					// this is the tile we are pre-fetching data for
					tile_id = read(0x2000 | (reg.vram_addr & 0x0FFF));
					break;

				case 2:
				{
					auto coarse_y = (reg.vram_addr & vram_coarse_y_mask) >> vram_coarse_y_shift;
					auto coarse_x = (reg.vram_addr & vram_coarse_x_mask) >> vram_coarse_x_shift;

					// construct an address into the attribute table
					// 0x2000 - base address of the name table or'ed with the nametable we wish to use
					// 0x03C0 - the offset into the nametable where attribute data lives
					auto addr = 0x23C0 | (reg.vram_addr & vram_nametable_mask)
						// shift the coarse_y into place and take the top three bits
					    // coarse_y is bits 5-9, so we end up with bit 7-9 shifted into bit 3-5
						| ((coarse_y << 1) & 0b111000)
						// shift the coarse_x into place and take the top three bits,
					    // moving bits 2-4 to 0-2
						| ((coarse_x >> 2) & 0b000111);

					auto attr = read(addr);

					// figure out how much we need to shift to read the attribute for this tile, either 0, 2, 4, or 6
					// use the high bit of the unused part of coarse_x/y
					auto shift = ((coarse_y << 1) & 0b100) | (coarse_x & 0b010);

					// save the attribute
					next_attribute = U8((attr >> shift) & 0b11);
					break;
				}

				case 4:
					next_pattern_lo = read(make_chrrom_addr() + 0);
					break;

				case 6:
					next_pattern_hi = read(make_chrrom_addr() + 8);
					break;

				case 7:
					increment_x();
					break;
				}

				if (cycle == 256)
					increment_y();

				if (cycle == 257)
					transfer_x();
			}

			if (cycle == 337)
			{
				shift();
				reload();
			}

			// these reads don't change any state, but some mappers may rely on these fetches for timing purposes
			if (cycle == 337 || cycle == 339)
				read(0x2000 | (reg.vram_addr & 0x0FFF));

			// the PPU transfers y coords every tick for several cycles in a row for some reason
			// we could probably just do a single transfer on cycle 304, but whatever
			if (scanline == 261 && cycle >= 280 && cycle < 305)
				transfer_y();
		}

		if (scanline == 241 && cycle == 1)
		{
			reg.ppustatus |= status_vblank;
			if (reg.ppuctrl & ctrl_nmi_flag)
				nes->cpu().nmi();
		}

		if (scanline == 261 && cycle == 1)
		{
			// clear vblank, sprite 0 hit, and sprite overflow; which is all the flags actually stored by status
			reg.ppustatus = 0;
		}

		// TODO: handle "sprites"

		// All preparation done, determine the value of the current pixel
		if (scanline < 240 && cycle >= 1 && cycle < 257)
		{
			U8 bg_palette_index = 0;
			if (background_rendering_enabled())
			{
				// the first 8 rows are skipped (or "transparent") if bit 1 of ppumask is unset
				if (cycle > 8 || (reg.ppumask & mask_show_leftmost_background))
				{
					U8 bit_offset = 0b1000'0000 >> reg.fine_x;
					U8 value = 0;

					value = (pattern_shifter_hi & bit_offset) > 0;
					bg_palette_index |= U8(value << 3);

					value = (pattern_shifter_lo & bit_offset) > 0;
					bg_palette_index |= U8(value << 2);

					value = (attribute_hi & bit_offset) > 0;
					bg_palette_index |= U8(value << 1);

					value = (attribute_lo & bit_offset) > 0;
					bg_palette_index |= U8(value << 0);
				}
			}

			U8 fg_palette_index = 0;
			if (sprite_rendering_enabled())
			{
				// TODO: select the foreground color
			}

			// TODO: select between the bg and fg color
			// for now, bg always wins!

			// the 0th entry of each palette group means use the "transparent" color (0)
			if ((bg_palette_index & 0b11) == 0)
				bg_palette_index = 0;

			U8 palette_index = bg_palette_index;

			// lookup the color this palette entry is mapped to
			auto index = read(0x3F00 + palette_index);
			nes->screen_out(cycle - 1, scanline, index);
		}

		// increment counters
		if (++cycle > 340)
		{
			cycle = 0;
			if (++scanline > 261)
			{
				scanline = 0;
				++frame;
			}
		}
	}

	U8 NesPpu::read(U16 addr) noexcept
	{
		if (cartridge)
		{
			auto value = cartridge->ppu_read(addr);
			if (value)
				return *value;
		}

		if (!VERIFY(!(addr < 0x2000), "The cart should have handled this range!"))
			return 0;

		if (addr < 0x3F00)
		{
			// $2000-$23FF Nametable 0
			// $2400-$27FF Nametable 1
			// $2800-$2BFF Nametable 2
			// $2C00-$2FFF Nametable 3
			// $3000-$3EFF mirror
			// always treat nametables as vertically mirrored and let the cartridge remap addr as needed
			// TODO: This may not work for some mappers... but we'll figure it out when we get there
			return nametable[(addr >> vram_nametable_shift) & 1][addr & 0x03FF];
		}

		if (addr < 0x4000)
		{
			// palette control
			addr &= 0x1F;

			// Addresses $3F10/$3F14/$3F18/$3F1C are mirrors of $3F00/$3F04/$3F08/$3F0C.
			switch (addr)
			{
			case 0x0010:
				addr = 0x0000;
				break;
			case 0x0014:
				addr = 0x0004;
				break;
			case 0x0018:
				addr = 0x0008;
				break;
			case 0x001C:
				addr = 0x000C;
				break;
			}

			return palettes[addr];
		}

		CHECK(false, "We shouldn't get here");
		return 0;
	}

	void NesPpu::write(U16 addr, U8 value) noexcept
	{
		if (cartridge && cartridge->ppu_write(addr, value))
		{
			return;
		}

		if (!VERIFY(!(addr < 0x2000), "The cart should have handled this range!"))
			return;

		if (addr < 0x3F00)
		{
			// $2000-$23FF Nametable 0
			// $2400-$27FF Nametable 1
			// $2800-$2BFF Nametable 2
			// $2C00-$2FFF Nametable 3
			// always treat nametables as vertically mirrored and let the cartridge remap addr as needed
			// TODO: This may not work for some mappers... but we'll figure it out when we get there
			nametable[(addr >> vram_nametable_shift) & 1][addr & 0x03FF] = value;
			return;
		}

		if (addr < 0x4000)
		{
			// palette control
			addr &= 0x1F;

			// Addresses $3F10/$3F14/$3F18/$3F1C are mirrors of $3F00/$3F04/$3F08/$3F0C.
			switch (addr)
			{
			case 0x0010:
				addr = 0x0000;
				break;
			case 0x0014:
				addr = 0x0004;
				break;
			case 0x0018:
				addr = 0x0008;
				break;
			case 0x001C:
				addr = 0x000C;
				break;
			}

			palettes[addr] = value;
			return;
		}

		CHECK(false, "We shouldn't get here");
	}

	U8 NesPpu::ppuctrl() noexcept
	{
		return latch;
	}

	void NesPpu::ppuctrl(U8 value) noexcept
	{
		reg.ppuctrl = latch = value;
		reg.tram_addr = (reg.tram_addr & ~vram_nametable_mask) | ((value & 3) << vram_nametable_shift);
	}

	U8 NesPpu::ppumask() noexcept
	{
		return latch;
	}

	void NesPpu::ppumask(U8 value) noexcept
	{
		reg.ppumask = latch = value;
	}

	U8 NesPpu::ppustatus() noexcept
	{
		// reading status not only returns the status register, it also resets the address latch and clears the vertical blank flag
		// the unused bits of status are filled with the latched value from other write operations
		latch = (0b11100000 & reg.ppustatus) | (0b00011111 & latch);

		reg.ppustatus &= U8(~status_vblank);
		reg.addr_latch = false;

		return latch;
	}

	void NesPpu::ppustatus(U8 value) noexcept
	{
		latch = value;
	}

	U8 NesPpu::oamaddr() noexcept
	{
		return latch;
	}

	void NesPpu::oamaddr(U8 value) noexcept
	{
		reg.oamaddr = latch = value;
	}

	U8 NesPpu::oamdata() noexcept
	{
		return oam[reg.oamaddr];
	}

	void NesPpu::oamdata(U8 value) noexcept
	{
		oam[reg.oamaddr++] = value;
	}

	U8 NesPpu::ppuscroll() noexcept
	{
		return latch;
	}

	void NesPpu::ppuscroll(U8 value) noexcept
	{
		if (!reg.addr_latch)
		{
			// write values for x
			reg.fine_x = value & 7;
			reg.tram_addr = (reg.tram_addr & ~vram_coarse_x_mask) | (value >> 3);
		}
		else
		{
			// write values for y
			reg.tram_addr =
				(reg.tram_addr & ~(vram_fine_y_mask | vram_coarse_y_mask)) |
				((value & 7) << vram_fine_y_shift) |
				((value >> 3) << vram_coarse_y_shift);
		}

		reg.addr_latch = !reg.addr_latch;
	}

	U8 NesPpu::ppuaddr() noexcept
	{
		return latch;
	}

	void NesPpu::ppuaddr(U8 value) noexcept
	{
		if (!reg.addr_latch)
		{
			reg.tram_addr = (reg.tram_addr & 0x00FF) | ((value & 0x7F) << 8);
		}
		else
		{
			reg.tram_addr = (reg.tram_addr & 0xFF00) | value;
			reg.vram_addr = reg.tram_addr;
		}

		reg.addr_latch = !reg.addr_latch;
	}

	U8 NesPpu::ppudata() noexcept
	{
		// non-palette data is buffered
		auto result = std::exchange(reg.ppudata, read(reg.vram_addr));

		// palette data is returned immediately
		if (reg.vram_addr >= 0x3F00)
			result = reg.ppudata;

		// the address is incremented on every read
		reg.vram_addr += (reg.ppuctrl & ctrl_vram_addr_inc ? 32 : 1);

		return result;
	}

	void NesPpu::ppudata(U8 value) noexcept
	{
		write(reg.vram_addr, value);

		// the address is incremented on every write
		reg.vram_addr += (reg.ppuctrl & ctrl_vram_addr_inc ? 32 : 1);
	}
}
