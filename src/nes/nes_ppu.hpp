#pragma once

#include <array>
#include <span>

#include "nes_types.hpp"

namespace nesem
{
	class Nes;
	class NesCartridge;

	// models a 2C02
	class NesPpu final
	{
	public:
		NesPpu(Nes *nes) noexcept;

		void reset() noexcept;
		void load_cartridge(NesCartridge *cartridge) noexcept;
		void clock() noexcept;

		// debugging help

		void draw_pattern_table(int index, U8 palette, const DrawFn &draw_pixel);
		void draw_name_table(int index, const DrawFn &draw_pixel);

		// PPU bus IO

		U8 read(U16 addr) noexcept;
		void write(U16 addr, U8 value) noexcept;

		// PPU registers

		U8 ppuctrl() noexcept;
		void ppuctrl(U8 value) noexcept;

		U8 ppumask() noexcept;
		void ppumask(U8 value) noexcept;

		U8 ppustatus() noexcept;
		void ppustatus(U8 value) noexcept;

		U8 oamaddr() noexcept;
		void oamaddr(U8 value) noexcept;

		U8 oamdata() noexcept;
		void oamdata(U8 value) noexcept;

		U8 ppuscroll() noexcept;
		void ppuscroll(U8 value) noexcept;

		U8 ppuaddr() noexcept;
		void ppuaddr(U8 value) noexcept;

		U8 ppudata() noexcept;
		void ppudata(U8 value) noexcept;

		struct ScrollInfo
		{
			int fine_x, fine_y;
			int coarse_x, coarse_y;
			int nt;
		};

		ScrollInfo get_scroll_info() const noexcept;

		const std::array<U8, 256> &get_oam() const noexcept;

	private:
		Nes *nes;

		// 0x0000-0x1FFF
		// pattern data on cart

		// 0x2000-0x2FFF, then mirrored from 0x3000-0x3EFF
		std::array<U8, 0x0400> nametable[2];

		// 0x3F00-0x3FFF
		std::array<U8, 32> palettes;
		std::array<U8, 256> oam;

		struct OAMSprite
		{
			U8 y = 0xFF;
			U8 index = 0xFF;
			U8 attrib = 0xFF;
			U8 x = 0xFF;

			// address of the low bits, add 8 to get the high bits
			// NOTE: this presumes the sprite intersects the current scanline
			U16 pattern_addr(U8 ppuctrl, int scanline) noexcept;
			bool flip_x() noexcept;
			bool flip_y() noexcept;
			bool bg_priority() noexcept;
			U8 palette() noexcept;
		};

		// working buffers for evaluating sprites for the next scanline
		std::array<OAMSprite, 8> evaluated_sprites;
		std::span<std::byte> evaluated_sprites_bytes = std::as_writable_bytes(std::span(evaluated_sprites));

		// the address sprite evaluation started at, usually 0
		U16 sprite_0_addr = 0xFFFF;

		// the oamaddr this sprite started at
		std::array<U8, 8> evaluated_sprite_addr;

		// the count of sprites that have passed evaluation so far
		size_t evaluated_sprite_count = 0;

		// the sprites that are active for the current scanline
		std::array<OAMSprite, 8> active_sprites;

		// the oamaddr this sprite started at
		std::array<U8, 8> active_sprite_addr;

		// the low bit chrrom data of the sprite for the next scanline
		std::array<U8, 8> active_sprite_lo;

		// the high bit chrrom data of the sprite for the next scanline
		std::array<U8, 8> active_sprite_hi;

		int sprite_size() noexcept;

		enum class SpriteEvaluationSteps
		{
			step1,
			step1a,
			step1b,
			step1c,
			step2,
			step3,
			step3a,
			step3b,
			step3c,
			step4,
		};
		SpriteEvaluationSteps sprite_evaluation_step = SpriteEvaluationSteps::step1;

		bool oam_clear = false;

		// https://wiki.nesdev.org/w/index.php?title=PPU_registers
		// Writing any value to any PPU port, even to the nominally read-only PPUSTATUS, will fill this latch. Reading
		// any readable port (PPUSTATUS, OAMDATA, or PPUDATA) also fills the latch with the bits read. Reading a nominally
		// "write-only" register returns the latch's current value, as do the unused bits of PPUSTATUS.
		U8 latch = 0;

		struct
		{
			U8 ppuctrl = 0; // write
			U8 ppumask = 0; // write
			U8 ppustatus = 0; // read

			// This is really an 8-bit address into OAM memory. using 16 bit addr so we can easily detect when
			// we've finished sprite evaluation, but oamdata write calls will wrap the address appropriately
			U16 oamaddr = 0; // write

			bool addr_latch = false;
			U16 tram_addr = 0; // writex2
			U16 vram_addr = 0; // writex2

			U8 fine_x = 0;
			U8 ppudata = 0;
		} reg;

		NesCartridge *cartridge = nullptr;

		U64 frame = 0;
		int cycle = 0;
		int scanline = 0;

		// buffer for read/write during rendering
		U8 next_tile_id = 0;
		U8 next_pattern_lo = 0;
		U8 next_pattern_hi = 0;
		U8 next_attribute = 0;

		void read_nt() noexcept;
		void read_at() noexcept;

		void reload() noexcept;
		void shift_bg() noexcept;
		void shift_fg() noexcept;
		U16 make_chrrom_addr() noexcept;
		void increment_x() noexcept;
		void increment_y() noexcept;
		void transfer_x() noexcept;
		void transfer_y() noexcept;

		bool rendering_enabled() noexcept;
		bool background_rendering_enabled() noexcept;
		bool sprite_rendering_enabled() noexcept;

		void prepare_background() noexcept;
		void prepare_foreground() noexcept;

		// rendering state

		U16 pattern_shifter_lo = 0;
		U16 pattern_shifter_hi = 0;
		U16 attribute_lo = 0;
		U16 attribute_hi = 0;
	};
}
