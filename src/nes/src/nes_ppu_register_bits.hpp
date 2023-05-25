#pragma once

#include <nes_types.hpp>

namespace nesem
{
	// clang-format off
	constexpr U16 vram_fine_y_mask      = 0b0'111'00'00000'00000;
	constexpr U16 vram_nametable_mask   = 0b0'000'11'00000'00000;
	constexpr U16 vram_nametable_y_mask = 0b0'000'10'00000'00000;
	constexpr U16 vram_nametable_x_mask = 0b0'000'01'00000'00000;
	constexpr U16 vram_coarse_y_mask    = 0b0'000'00'11111'00000;
	constexpr U16 vram_coarse_x_mask    = 0b0'000'00'00000'11111;

	constexpr U16 vram_fine_y_shift    = 12;
	constexpr U16 vram_nametable_shift = 10;
	constexpr U16 vram_coarse_y_shift  = 5;
	constexpr U16 vram_coarse_x_shift  = 0;

	constexpr U8 status_vblank          = 0b100'00000;
	constexpr U8 status_sprite0_hit     = 0b010'00000;
	constexpr U8 status_sprite_overflow = 0b001'00000;
	constexpr U8 status_unused          = 0b000'11111;

	constexpr U8 ctrl_nmi_flag       = 0b1000'0000;
	// constexpr U8 ctrl_master_flag = 0b0100'0000; // isn't normally used and is always 0 on a stock NES
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
}
