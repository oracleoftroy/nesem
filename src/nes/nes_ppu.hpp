#pragma once

#include <array>

#include "nes_types.hpp"

namespace nesem
{
	class Nes;
	class NesCartridge;

	struct NesPatternTable
	{
		// pattern table is a 16x16 table of 8x8 tiles
		// each entry is 2 bits each, packed 4 per byte
		// in other words, we need 2 bytes per tile per row
		// thus, a 16x16 grid of 8x2 bytes
		std::array<U8, 16 * 16 * 8 * 2> table;

		U8 read_pixel(U16 x, U16 y, U8 palette) const noexcept;
		void write_pixel(U16 x, U16 y, U8 entry) noexcept;
	};

	struct NesNameTable
	{
		std::array<U8, 256 * 240> table;

		U8 read_pixel(U16 x, U16 y) const noexcept;
		void write_pixel(U16 x, U16 y, U8 palette) noexcept;
	};

	// models a 2C02
	class NesPpu final
	{
	public:
		NesPpu(Nes *nes) noexcept;

		void reset() noexcept;
		void load_cartridge(NesCartridge *cartridge) noexcept;
		bool clock() noexcept;

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

		U8 oamdata() const noexcept;
		void oamdata(U8 value) noexcept;

		U8 ppuscroll() noexcept;
		void ppuscroll(U8 value) noexcept;

		U8 ppuaddr() noexcept;
		void ppuaddr(U8 value) noexcept;

		U8 ppudata() noexcept;
		void ppudata(U8 value) noexcept;

		struct OAMSprite
		{
			U8 y = 0xFF;
			U8 index = 0xFF;
			U8 attrib = 0xFF;
			U8 x = 0xFF;

			// the oamaddr this sprite started at
			U8 addr = 0xFF;

			// the low bit chrrom data of the sprite for the next scanline
			U8 lo = 0xFF;

			// the high bit chrrom data of the sprite for the next scanline
			U8 hi = 0xFF;

			// address of the low bits, add 8 to get the high bits
			// NOTE: this presumes the sprite intersects the provided scanline
			U16 pattern_addr(U8 ppuctrl, int scanline) noexcept;
			void read_lo(NesPpu &ppu, U8 ppuctrl, int scanline) noexcept;
			void read_hi(NesPpu &ppu, U8 ppuctrl, int scanline) noexcept;
			bool flip_x() noexcept;
			bool flip_y() noexcept;
			bool bg_priority() noexcept;
			U8 palette_index() noexcept;
		};

	private:
		Nes *nes;

		// 0x0000-0x1FFF
		// pattern data on cart

		// 0x2000-0x2FFF, then mirrored from 0x3000-0x3EFF
		std::array<U8, 0x0400> nametable[2];

		// 0x3F00-0x3FFF
		std::array<U8, 32> palettes{};
		std::array<U8, 256> oam;

		// working buffer for evaluating sprites for the next scanline
		std::array<U8, 8 * 4> evaluated_sprites;

		// the address sprite evaluation started at, usually 0
		U16 sprite_0_addr = 0xFFFF;

		// the oamaddr this sprite started at
		std::array<U8, 8> evaluated_sprite_addr;

		// the count of sprites that have passed evaluation so far
		size_t evaluated_sprite_count = 0;

		// the sprites that are active for the current scanline
		std::array<OAMSprite, 8> active_sprites;

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

		U64 tick = 0; // number of times PPU has been clocked since last reset
		U64 frame = 0; // number of full frames completed
		int cycle = 0; // current cycle within the current scanline (roughly acts as an x coordinate)
		int scanline = 0; // current scanline (roughly acts as a y coordinate)

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

		U8 get_color_index() noexcept;
		NesColorEmphasis color_emphasis() const noexcept;
		U8 apply_grayscale(U8 color_index) const noexcept;

		// rendering state

		U16 pattern_shifter_lo = 0;
		U16 pattern_shifter_hi = 0;
		U16 attribute_lo = 0;
		U16 attribute_hi = 0;

		U8 read_internal(U16 addr) const noexcept;

	public:
		// debugging help

		struct ScrollInfo
		{
			U8 fine_x, fine_y;
			U8 coarse_x, coarse_y;
			U8 nt;
		};

		U8 peek(U16 addr) const noexcept;

		U64 current_tick() const noexcept;
		U64 current_frame() const noexcept;
		int current_scanline() const noexcept;
		int current_cycle() const noexcept;
		ScrollInfo get_scroll_info() const noexcept;
		const std::array<U8, 256> &get_oam() const noexcept;
		const std::array<OAMSprite, 8> &get_active_sprites() const noexcept;

		// get the current pattern table for visualization / debugging
		NesPatternTable read_pattern_table(int index) const noexcept;

		// get the current name table based on the provided pre-calculated pattern tables
		NesNameTable read_name_table(int index, const std::array<NesPatternTable, 2> &pattern) const noexcept;
	};
}
