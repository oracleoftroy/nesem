#include "nes_ppu.hpp"

#include "nes.hpp"

#include <util/logging.hpp>

namespace
{
	constexpr uint8_t reverse_bits(uint8_t value) noexcept
	{
		// http://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith64Bits
		return uint8_t(((value * 0x80200802ULL) & 0x0884422110ULL) * 0x0101010101ULL >> 32);
	}
}

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
	constexpr U8 ctrl_sprite_8x16    = 0b0010'0000;
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

	U16 NesPpu::OAMSprite::pattern_addr(U8 ppuctrl, int scanline) noexcept
	{
		U16 sprite_row = U16(scanline - y);

		U16 row = sprite_row;
		if (flip_y())
			row = 7 - row;

		// force row into the range of 0-7
		row &= 7;

		if (ppuctrl & ctrl_sprite_8x16)
		{
			// 8x16 sprite
			U16 cell = index & 0xFE;

			// if we are rendering the bottom part of the sprite, increment the cell
			// NOTE: that we need to account for whether the sprite is vertically flipped, so we increment
			// only if we are in the first 8 rows of a flipped cell or the last 8 rows of an unflipped cell
			if ((sprite_row < 8 && flip_y()) || (sprite_row >= 8 && !flip_y()))
				++cell;

			return ((index & 1) << 12) | (cell << 4) | row;
		}
		else
		{
			// 8x8 sprite
			return ((ppuctrl & ctrl_sprite_addr) ? 0x1000 : 0) | (index << 4) | row;
		}
	}

	void NesPpu::OAMSprite::read_lo(NesPpu &ppu, U8 ppuctrl, int scanline) noexcept
	{
		lo = ppu.read(pattern_addr(ppuctrl, scanline) + 0);

		if (flip_x())
			lo = reverse_bits(lo);
	}

	void NesPpu::OAMSprite::read_hi(NesPpu &ppu, U8 ppuctrl, int scanline) noexcept
	{
		hi = ppu.read(pattern_addr(ppuctrl, scanline) + 8);

		if (flip_x())
			hi = reverse_bits(hi);
	}

	bool NesPpu::OAMSprite::flip_x() noexcept
	{
		return (attrib & 0b0100'0000) != 0;
	}

	bool NesPpu::OAMSprite::flip_y() noexcept
	{
		return (attrib & 0b1000'0000) != 0;
	}

	bool NesPpu::OAMSprite::bg_priority() noexcept
	{
		return (attrib & 0b0010'0000) != 0;
	}

	U8 NesPpu::OAMSprite::palette_index() noexcept
	{
		U8 bit_lo = (lo & 0x80) > 0;
		U8 bit_hi = (hi & 0x80) > 0;
		U8 palette = 4 | (attrib & 3);

		return (palette << 2) | (bit_hi << 1) | bit_lo;
	}

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

		tick = 0;
		frame = 0;
		cycle = 0;
		scanline = 0;
	}

	void NesPpu::load_cartridge(NesCartridge *cart) noexcept
	{
		this->cartridge = cart;
	}

	U64 NesPpu::current_tick() const noexcept
	{
		return tick;
	}

	int NesPpu::current_scanline() const noexcept
	{
		return scanline;
	}

	int NesPpu::current_cycle() const noexcept
	{
		return cycle;
	}

	NesPpu::ScrollInfo NesPpu::get_scroll_info() const noexcept
	{
		return {
			.fine_x = reg.fine_x,
			.fine_y = U8((reg.vram_addr & vram_fine_y_mask) >> vram_fine_y_shift),
			.coarse_x = U8((reg.vram_addr & vram_coarse_x_mask) >> vram_coarse_x_shift),
			.coarse_y = U8((reg.vram_addr & vram_coarse_y_mask) >> vram_coarse_y_shift),
			.nt = U8((reg.vram_addr & vram_nametable_mask) >> vram_nametable_shift),
		};
	}

	const std::array<U8, 256> &NesPpu::get_oam() const noexcept
	{
		return oam;
	}

	const std::array<NesPpu::OAMSprite, 8> &NesPpu::get_active_sprites() const noexcept
	{
		return active_sprites;
	}

	U8 NesPatternTable::read_pixel(U16 x, U16 y, U8 palette) const noexcept
	{
		auto x_shift = (x & 0b11) << 1;
		auto x_pos = x >> 2;
		auto index = y * 16 * 2 + x_pos;

		return (palette << 2) | ((table[index] >> x_shift) & 0b11);
	}

	void NesPatternTable::write_pixel(U16 x, U16 y, U8 entry) noexcept
	{
		auto x_pos = x >> 2;
		auto index = y * 16 * 2 + x_pos;

		auto x_shift = (x & 0b11) << 1;
		U8 mask = 0b11 << x_shift;

		table[index] = (table[index] & ~mask) | ((entry << x_shift) & mask);
	}

	NesPatternTable NesPpu::read_pattern_table(int index) noexcept
	{
		NesPatternTable result;

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
						U8 palette_index = ((tile_hi & bit) != 0) << 1 | ((tile_lo & bit) != 0);

						result.write_pixel(tile_x * 8 + col, tile_y * 8 + row, palette_index);
					}
				}
			}
		}

		return result;
	}

	U8 NesNameTable::read_pixel(U16 x, U16 y) const noexcept
	{
		return table[y * 256 + x];
	}

	void NesNameTable::write_pixel(U16 x, U16 y, U8 palette) noexcept
	{
		table[y * 256 + x] = palette;
	}

	NesNameTable NesPpu::read_name_table(int index, const std::array<NesPatternTable, 2> &pattern) noexcept
	{
		NesNameTable result;
		U16 pattern_index = (reg.ppuctrl & ctrl_pattern_addr) != 0 ? 1 : 0;

		for (U16 tile_y = 0; tile_y < 30; ++tile_y)
		{
			for (U16 tile_x = 0; tile_x < 32; ++tile_x)
			{
				index = index ? 3 : 0;
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
					for (U16 col = 0; col < 8; ++col)
					{
						auto tile = pattern[pattern_index].read_pixel((nt & 0xF) * 8 + col, ((nt >> 4) & 0xF) * 8 + row, attr);
						result.write_pixel(tile_x * 8 + col, tile_y * 8 + row, read(0x3F00 + tile));
					}
				}
			}
		}

		return result;
	}

	void NesPpu::reload() noexcept
	{
		pattern_shifter_lo = (pattern_shifter_lo & 0xFF00) | next_pattern_lo;
		pattern_shifter_hi = (pattern_shifter_hi & 0xFF00) | next_pattern_hi;

		// handle the attribute data like the pattern so that fine_x works the same
		attribute_lo = U16((attribute_lo & 0xFF00) | ((next_attribute & 1) ? 0xFF : 0));
		attribute_hi = U16((attribute_hi & 0xFF00) | ((next_attribute & 2) ? 0xFF : 0));
	}

	void NesPpu::shift_bg() noexcept
	{
		if (background_rendering_enabled())
		{
			pattern_shifter_lo <<= 1;
			pattern_shifter_hi <<= 1;

			attribute_lo <<= 1;
			attribute_hi <<= 1;
		}
	}

	void NesPpu::shift_fg() noexcept
	{
		if (sprite_rendering_enabled())
		{
			for (size_t i = 0; i < size(active_sprites); ++i)
			{
				if (active_sprites[i].x > 0 && active_sprites[i].x != 255)
					--active_sprites[i].x;
				else
				{
					active_sprites[i].lo <<= 1;
					active_sprites[i].hi <<= 1;
				}
			}
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
		U16 pattern_start = (reg.ppuctrl & ctrl_pattern_addr) != 0 ? 0x1000 : 0;
		return pattern_start | (next_tile_id << 4) | ((reg.vram_addr & vram_fine_y_mask) >> vram_fine_y_shift);
	}

	void NesPpu::read_nt() noexcept
	{
		next_tile_id = read(0x2000 | (reg.vram_addr & 0x0FFF));
	}

	void NesPpu::read_at() noexcept
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
	}

	bool NesPpu::clock() noexcept
	{
		bool frame_complete = false;

		// odd frame skip: the very first cycle of an odd frame is skipped if rendering is enabled
		if (scanline == 0 && cycle == 0 && (frame & 1) == 1 && rendering_enabled())
			cycle = 1;

		prepare_background();
		prepare_foreground();

		if (scanline == 241 && cycle == 1)
		{
			reg.ppustatus |= status_vblank;
			if (reg.ppuctrl & ctrl_nmi_flag)
				nes->cpu().nmi();

			nes->frame_complete();
			frame_complete = true;
		}

		if (scanline == 261 && cycle == 1)
		{
			// clear vblank, sprite 0 hit, and sprite overflow; which is all the flags actually stored by status
			reg.ppustatus = 0;
		}

		// All preparation done, determine the value of the current pixel
		if (scanline < 240 && cycle >= 1 && cycle < 257)
		{
			U8 bg_palette_index = 0;
			if (background_rendering_enabled())
			{
				// the first 8 rows are skipped (or "transparent") if bit 1 of ppumask is unset
				if (cycle > 8 || (reg.ppumask & mask_show_leftmost_background))
				{
					U16 bit_offset = 0b1000'0000'0000'0000 >> reg.fine_x;
					U8 value = 0;

					value = (attribute_hi & bit_offset) > 0;
					bg_palette_index |= U8(value << 3);

					value = (attribute_lo & bit_offset) > 0;
					bg_palette_index |= U8(value << 2);

					value = (pattern_shifter_hi & bit_offset) > 0;
					bg_palette_index |= U8(value << 1);

					value = (pattern_shifter_lo & bit_offset) > 0;
					bg_palette_index |= U8(value << 0);
				}
			}

			U8 fg_palette_index = 0;
			U8 bg_priority = false;
			U8 fg_id = 0xFF;

			if (sprite_rendering_enabled())
			{
				// the first 8 rows are skipped (or "transparent") if bit 1 of ppumask is unset
				if (cycle > 8 || (reg.ppumask & mask_show_leftmost_sprites))
				{
					for (size_t i = 0; i < size(active_sprites); ++i)
					{
						if (active_sprites[i].x == 0)
						{
							fg_palette_index = active_sprites[i].palette_index();
							bg_priority = active_sprites[i].bg_priority();
							fg_id = active_sprites[i].addr;

							// if this sprite is not transparent, we are done
							// the first entry of each palette is 'transparent'
							if (fg_palette_index & 3)
								break;
						}
					}
				}
			}

			// We've determined which bg and fg color is relevant, now figure out which one actually gets rendered
			U8 palette_index = 0;

			// rules when background is transparent
			if ((bg_palette_index & 0b11) == 0)
			{
				// both transparent, use the transparent color (0)
				if ((fg_palette_index & 0b11) == 0)
					palette_index = 0;

				// non-transparent foreground beats transparent background
				else
					palette_index = fg_palette_index;
			}

			// non-transparent background beats transparent foreground
			else if ((fg_palette_index & 0b11) == 0)
				palette_index = bg_palette_index;

			// neither fg or bg are transparent
			else
			{
				// the sprite will tell us who wins the tie
				palette_index = bg_priority ? bg_palette_index : fg_palette_index;

				// both background and foreground are non-transparent, so check for a sprite 0 hit
				if (fg_id == sprite_0_addr)
					reg.ppustatus |= status_sprite0_hit;
			}

			// lookup the color this palette entry is mapped to
			auto index = read(0x3F00 + palette_index);
			nes->screen_out(cycle - 1, scanline, index);
			shift_fg();
		}

		// increment counters
		++tick;

		if (++cycle > 340)
		{
			cycle = 0;
			if (++scanline > 261)
			{
				scanline = 0;
				++frame;
			}
		}

		return frame_complete;
	}

	void NesPpu::prepare_background() noexcept
	{
		// during these scanlines, we prepare background data for rendering. Scanline 261 (or -1) prefetches data rendered during the first scanline
		if (scanline < 240 || scanline == 261)
		{
			if ((cycle >= 1 && cycle <= 257) || (cycle >= 321 && cycle <= 336))
			{
				if (cycle > 1)
					shift_bg();

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
					read_nt();
					break;

				case 2:
					read_at();
					break;

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

			if (cycle >= 257 && cycle < 321)
			{
				// explictly set to 0 every tick of these cycles
				// maybe this should be considered part of the foreground handling, but this happens on the pre-render scanline as well, so it is here
				reg.oamaddr = 0;
			}

			if (cycle == 337)
			{
				shift_bg();
				reload();
			}

			// extra reads of the nametable, some mappers may rely on these fetches for timing purposes
			if (cycle == 337 || cycle == 339)
				read_nt();

			// the PPU transfers y coords every tick for several cycles in a row for some reason
			// we could probably just do a single transfer on cycle 304, but whatever
			if (scanline == 261 && cycle >= 280 && cycle < 305)
				transfer_y();
		}
	}

	void NesPpu::prepare_foreground() noexcept
	{
		// Nesdev
		// 1: Cycles 1-64: Secondary OAM (32-byte buffer for current sprites on scanline) is initialized to $FF - attempting to read $2004 will return $FF.
		// Internally, the clear operation is implemented by reading from the OAM and writing into the secondary OAM as usual, only a signal is active that makes the read always return $FF.
		// end Nesdev

		// prepare foreground sprites for next scanline. No prep occurs for scanline 261 (a.k.a scanline -1),
		// thus no sprite can be drawn on scanline 0
		if (scanline < 240)
		{
			// during these cycles we are clearing our selected sprite buffer. This also
			// configures oamdata to always return 0xFF regardless of what is stored
			if (cycle == 1)
				oam_clear = true;
			else if (cycle == 65)
			{
				oam_clear = false;
				evaluated_sprite_count = 0;
				sprite_evaluation_step = SpriteEvaluationSteps::step1;

				// we allow oamaddr to overflow beyond its 8-bit range so that we can detect when we've finished evaluating all sprites
				// make sure it isn't currently in an overflow state just before entering sprite evaluation
				reg.oamaddr &= 0xFF;

				// remember where oamaddr started for sprite 0 evaluation
				// NesDev wiki is a bit unclear. The page documenting sprite evaluation (https://wiki.nesdev.org/w/index.php?title=PPU_sprite_evaluation)
				// has no mention of oamaddr or where sprite 0 starts, but the registers page (https://wiki.nesdev.org/w/index.php?title=PPU_registers#OAMADDR)
				// indicates that sprite evaluation begins whereever oamaddr happens to point, and sprite 0 thus can be other than the sprite at oamaddr 0
				// and potentially even a misaligned starting address. This also skips any sprite that is stored before oamaddr
				// TODO: find out if any games actually require this behavior and test
				sprite_0_addr = reg.oamaddr;
			}

			// clear our sprite data
			if (cycle >= 1 && cycle < 65 && (cycle & 1) == 1)
				evaluated_sprites[cycle >> 1] = 0xFF;

			// sprite evaluation: figure out which sprites (up to 8) should be displayed
			else if (cycle >= 65 && cycle < 257)
			{
				// sprite evaluation starts wherever oamaddr happens to be pointing (usually 0) and continues
				// until it evaluates all 64 sprites (4 bytes per sprite, thus 256 bytes total)
				// If the app misaligns oamaddr, so be it

				// Nesdev
				// 2: Cycles 65-256: Sprite evaluation
				// 	- On odd cycles, data is read from (primary) OAM
				// 	- On even cycles, data is written to secondary OAM (unless secondary OAM is full, in which case it will read the value in secondary OAM instead)
				// end Nesdev

				switch (sprite_evaluation_step)
				{
					using enum SpriteEvaluationSteps;
				case step1:
					// 	1. Starting at n = 0, read a sprite's Y-coordinate (OAM[n][0], copying it to the next open slot in secondary OAM (unless 8 sprites have been found, in which case the write is ignored).
					if (evaluated_sprite_count < 8)
					{
						auto y = evaluated_sprites[evaluated_sprite_count * 4] = oam[reg.oamaddr];
						evaluated_sprite_addr[evaluated_sprite_count] = U8(reg.oamaddr);

						auto pos = scanline - y;

						if (pos >= 0 && pos < sprite_size())
							sprite_evaluation_step = step1a;
						else
							sprite_evaluation_step = step2;
					}
					break;

					// 1a. If Y-coordinate is in range, copy remaining bytes of sprite data (OAM[n][1] thru OAM[n][3]) into secondary OAM.
				case step1a:
					evaluated_sprites[evaluated_sprite_count * 4 + 1] = oam[reg.oamaddr + 1];
					sprite_evaluation_step = step1b;
					break;

				case step1b:
					evaluated_sprites[evaluated_sprite_count * 4 + 2] = oam[reg.oamaddr + 2];
					sprite_evaluation_step = step1c;
					break;

				case step1c:
					evaluated_sprites[evaluated_sprite_count * 4 + 3] = oam[reg.oamaddr + 3];
					++evaluated_sprite_count;
					sprite_evaluation_step = step2;
					break;

				case step2:
					// 	2. Increment n
					// 		2a. If n has overflowed back to zero (all 64 sprites evaluated), go to 4
					// 		2b. If less than 8 sprites have been found, go to 1
					// 		2c. If exactly 8 sprites have been found, disable writes to secondary OAM because it is full. This causes sprites in back to drop out.
					reg.oamaddr += 4;

					if (reg.oamaddr > 255)
						sprite_evaluation_step = step4;
					else if (evaluated_sprite_count < 8)
						sprite_evaluation_step = step1;
					else
						sprite_evaluation_step = step3;

					break;

				case step3:
					// 	3. Starting at m = 0, evaluate OAM[n][m] as a Y-coordinate.
					// 		3a. If the value is in range, set the sprite overflow flag in $2002 and read the next 3 entries of OAM (incrementing 'm' after each byte and incrementing 'n' when 'm' overflows); if m = 3, increment n
					// 		3b. If the value is not in range, increment n and m (without carry). If n overflows to 0, go to 4; otherwise go to 3
					// 			- The m increment is a hardware bug - if only n was incremented, the overflow flag would be set whenever more than 8 sprites were present on the same scanline, as expected.

					if (auto pos = scanline - oam[reg.oamaddr++];
						pos >= 0 && pos < sprite_size())
					{
						reg.ppustatus |= status_sprite_overflow;
						sprite_evaluation_step = step3a;
					}
					else
					{
						// OAM increment bug: should only increment by 4, but the PPU hardware messed up this case
						// we simulate this by already incremented m when reading y
						reg.oamaddr += 4;
						if (reg.oamaddr > 255)
							sprite_evaluation_step = step4;
					}
					break;

				case step3a:
					++reg.oamaddr;
					sprite_evaluation_step = step3b;
					break;

				case step3b:
					++reg.oamaddr;
					sprite_evaluation_step = step3c;
					break;

				case step3c:
					++reg.oamaddr;
					if (reg.oamaddr > 255)
						sprite_evaluation_step = step4;
					else
						sprite_evaluation_step = step3;
					break;

				case step4:
					// 	4. Attempt (and fail) to copy OAM[n][0] into the next free slot in secondary OAM, and increment n (repeat until HBLANK is reached)
					reg.oamaddr += 4;
					break;
				}
			}

			// load the sprite data into our shifters for the next scanline
			// all data needed for rendering the sprite is copied over to active_sprites, as the PPU modifies evaluation data during the visible portion of a scanline
			else if (cycle >= 257 && cycle < 321)
			{
				// Nesdev
				// 3: Cycles 257-320: Sprite fetches (8 sprites total, 8 cycles per sprite)
				// 	1-4: Read the Y-coordinate, tile number, attributes, and X-coordinate of the selected sprite from secondary OAM
				// 	5-8: Read the X-coordinate of the selected sprite from secondary OAM 4 times (while the PPU fetches the sprite tile data)
				// 	For the first empty sprite slot, this will consist of sprite #63's Y-coordinate followed by 3 $FF bytes; for subsequent empty sprite slots, this will be four $FF bytes
				// end Nesdev

				int step = cycle - 257;
				auto index = step / 8;

				switch (step & 7)
				{
				case 0:
					active_sprites[index].addr = evaluated_sprite_addr[index];
					active_sprites[index].y = evaluated_sprites[index * 4 + 0];

					// dummy nametable read, some carts might use this for timing purposes
					read_nt();
					break;

				case 1:
					active_sprites[index].index = evaluated_sprites[index * 4 + 1];
					break;

				case 2:
					active_sprites[index].attrib = evaluated_sprites[index * 4 + 2];

					// dummy attribute table read, some carts might use this for timing purposes
					read_at();
					break;

				case 3:
					active_sprites[index].x = evaluated_sprites[index * 4 + 3];
					break;

				case 4:
					active_sprites[index].read_lo(*this, reg.ppuctrl, scanline);
					break;

				case 6:
					active_sprites[index].read_hi(*this, reg.ppuctrl, scanline);
					break;
				}
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
		// during oam clear (cycles 1-64 of visible scanlines), this reads 0xFF regardless of what is stored
		if (oam_clear)
			return 0xFF;

		return oam[reg.oamaddr & 0xFF];
	}

	void NesPpu::oamdata(U8 value) noexcept
	{
		oam[reg.oamaddr++ & 0xFF] = value;
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

	int NesPpu::sprite_size() noexcept
	{
		return reg.ppuctrl & ctrl_sprite_8x16 ? 16 : 8;
	}
}
