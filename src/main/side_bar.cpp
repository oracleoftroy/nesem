#include "side_bar.hpp"

#include <functional>
#include <numbers>

#include <fmt/format.h>
#include <nes_cartridge.hpp>

#include "color_palette.hpp"
#include "nes_app.hpp"
#include "text.hpp"

#include <ui/app.hpp>
#include <ui/renderer.hpp>
#include <util/logging.hpp>

namespace app
{
	SideBar::SideBar(ui::App &app, cm::Recti area)
		: texture(app.create_texture(size(area))),
		  nes_pattern_textures{app.create_texture({128, 128}), app.create_texture({128, 128})},
		  nes_nametable_textures{app.create_texture({256, 240}), app.create_texture({256, 240})},
		  nes_sprite_texture{app.create_texture({256, 240})},
		  area(area)
	{
	}

	void SideBar::update(DebugMode mode, const nesem::Nes &nes, nesem::U8 current_palette, const ColorPalette &colors)
	{
		switch (mode)
		{
			using enum DebugMode;
		case none:
			break;

		case bg_info:
			draw_ppu_visualizer(nes, colors);
			break;

		case fg_info:
			draw_ppu_info(nes, current_palette, colors);
			break;

		case cpu_info:
			draw_cpu_info(nes);
			break;
		}
	}

	void SideBar::draw_cpu_info(const nesem::Nes &nes)
	{
		const auto state = nes.cpu().state();

		auto [canvas, lock] = texture.lock();
		canvas.fill({22, 22, 22});

		auto pos = cm::Point2{4, 4};
		draw_string(canvas, {255, 255, 255}, "CPU Registers", pos);

		pos.y += 14;
		{
			using enum nesem::ProcessorStatus;
			draw_string(canvas, {255, 255, 255},
				fmt::format("Flags:  {} {} {} {} {} {} {} {}  S: {:02X}",
					state.P.is_set(N) ? 'N' : '-',
					state.P.is_set(V) ? 'V' : '-',
					state.P.is_set(E) ? 'E' : '-',
					state.P.is_set(B) ? 'B' : '-',
					state.P.is_set(D) ? 'D' : '-',
					state.P.is_set(I) ? 'I' : '-',
					state.P.is_set(Z) ? 'Z' : '-',
					state.P.is_set(C) ? 'C' : '-',
					state.S),
				pos);
		}

		pos.y += 14;
		draw_string(canvas, {255, 255, 255}, fmt::format("PC: {}   A: {:02X}  X: {:02X}  Y: {:02X}", state.PC, state.A, state.X, state.Y), pos);

		pos.y += 16;

		const auto canvas_size = canvas.size();
		canvas.draw_line({222, 222, 222}, pos, pos + cm::Point2{canvas_size.w - pos.x - 4, 0});

		if (const auto *cartridge = nes.cartridge();
			!cartridge)
		{
			pos.y += 16;
			draw_string(canvas, {255, 255, 255}, "No Cartridge loaded", pos);
		}
		else
		{
			if (cartridge->rom().v2)
			{
				const auto &v2 = *cartridge->rom().v2;

				pos.y += 16;
				draw_string(canvas, {255, 255, 255}, fmt::format("Mapper: {:03}  Submapper: {}", v2.pcb.mapper, v2.pcb.submapper), pos);

				pos.y += 12;
				draw_string(canvas, {255, 255, 255}, fmt::format("PRG ROM size: {0}K ({1})", v2.prgrom.size / 1024, v2.prgrom.size), pos);

				if (v2.prgram)
				{
					pos.y += 12;
					draw_string(canvas, {255, 255, 255}, fmt::format("PRG RAM size: {0}K ({1})", v2.prgram.value() / 1024, v2.prgram.value()), pos);
				}
				if (v2.prgnvram)
				{
					pos.y += 12;
					draw_string(canvas, {255, 255, 255}, fmt::format("PRG NVRAM size: {0}K ({1})", v2.prgnvram.value() / 1024, v2.prgnvram.value()), pos);
				}
				if (v2.chrrom)
				{
					pos.y += 12;
					draw_string(canvas, {255, 255, 255}, fmt::format("CHR ROM size: {0}K ({1})", v2.chrrom->size / 1024, v2.chrrom->size), pos);
				}
				if (v2.chrram)
				{
					pos.y += 12;
					draw_string(canvas, {255, 255, 255}, fmt::format("CHR RAM size: {0}K ({1})", v2.chrram.value() / 1024, v2.chrram.value()), pos);
				}
				if (v2.chrnvram)
				{
					pos.y += 12;
					draw_string(canvas, {255, 255, 255}, fmt::format("CHR NVRAM size: {0}K ({1})", v2.chrnvram.value() / 1024, v2.chrnvram.value()), pos);
				}
			}
			else
			{
				const auto &v1 = cartridge->rom().v1;

				pos.y += 16;
				draw_string(canvas, {255, 255, 255}, fmt::format("Mapper: {:03}", v1.mapper), pos);

				pos.y += 12;
				draw_string(canvas, {255, 255, 255}, fmt::format("PRG ROM size: {0}K ({1})", v1.prg_rom_size * 16, v1.prg_rom_size * nesem::bank_16k), pos);

				if (v1.chr_rom_size == 0)
				{
					pos.y += 12;
					draw_string(canvas, {255, 255, 255}, fmt::format("CHR RAM size: 8K ({})", nesem::bank_8k), pos);
				}
				else
				{
					pos.y += 12;
					draw_string(canvas, {255, 255, 255}, fmt::format("CHR ROM size: {0}K ({1})", v1.chr_rom_size * 8, v1.chr_rom_size * nesem::bank_8k), pos);
				}
			}

			auto cpu_rom_area = cm::Recti{pos.x, canvas_size.h - 512 - 4, (canvas_size.w - (pos.x + 48)) / 4, 512};
			auto prg_rom_area = cpu_rom_area + cm::Point2{cpu_rom_area.w + 12};
			auto ppu_chr_area = prg_rom_area + cm::Point2{prg_rom_area.w + 20};
			auto rom_chr_area = ppu_chr_area + cm::Point2{ppu_chr_area.w + 12};

			{
				auto label_area = cpu_rom_area;
				label_area.y -= 12;
				label_area.h = 8;
				draw_string_centered(canvas, {255, 255, 255}, "CPU", label_area);
			}
			{
				auto label_area = prg_rom_area;
				label_area.y -= 12;
				label_area.h = 8;
				draw_string_centered(canvas, {255, 255, 255}, "Cart", label_area);
			}
			{
				auto cpu_label_area = cm::rect(top_left(cpu_rom_area), bottom_right(prg_rom_area));
				cpu_label_area.y -= 32;
				cpu_label_area.h = 16;
				draw_string_centered(canvas, {255, 255, 255}, "PRG-ROM", cpu_label_area);
				canvas.draw_line({222, 222, 222}, bottom_left(cpu_label_area), bottom_right(cpu_label_area));
			}

			{
				auto label_area = ppu_chr_area;
				label_area.y -= 12;
				label_area.h = 8;
				draw_string_centered(canvas, {255, 255, 255}, "PPU", label_area);
			}
			{
				auto label_area = rom_chr_area;
				label_area.y -= 12;
				label_area.h = 8;
				draw_string_centered(canvas, {255, 255, 255}, "Cart", label_area);
			}
			{
				auto ppu_label_area = cm::rect(top_left(ppu_chr_area), bottom_right(rom_chr_area));
				ppu_label_area.y -= 32;
				ppu_label_area.h = 16;
				draw_string_centered(canvas, {255, 255, 255}, "CHR memory", ppu_label_area);
				canvas.draw_line({222, 222, 222}, bottom_left(ppu_label_area), bottom_right(ppu_label_area));
			}

			canvas.fill_rect({88, 88, 88}, cpu_rom_area);
			canvas.draw_rect({222, 222, 222}, cpu_rom_area);

			canvas.fill_rect({88, 88, 88}, prg_rom_area);
			canvas.draw_rect({222, 222, 222}, prg_rom_area);

			canvas.fill_rect({88, 88, 88}, ppu_chr_area);
			canvas.draw_rect({222, 222, 222}, ppu_chr_area);

			canvas.fill_rect({88, 88, 88}, rom_chr_area);
			canvas.draw_rect({222, 222, 222}, rom_chr_area);

			constexpr auto prg_bank_color = [](auto bank, float l = 0.75) {
				constexpr auto phi = std::numbers::phi_v<float>;
				constexpr auto golden_angle = 360.0f / (phi * phi);
				constexpr auto start_hue = 120.0f;
				return to_color(to_rgb(cm::ColorHSL{.h = start_hue + bank * golden_angle, .s = 0.75f, .l = l}));
			};
			constexpr auto chr_bank_color = [](auto bank, float l = 0.75) {
				constexpr auto phi = std::numbers::phi_v<float>;
				constexpr auto golden_angle = 360.0f / (phi * phi);
				constexpr auto start_hue = 220.0f;
				return to_color(to_rgb(cm::ColorHSL{.h = start_hue + bank * golden_angle, .s = 0.65f, .l = l}));
			};

			constexpr auto draw_string_centered_shaded = [](ui::Canvas &canvas, std::string_view txt, const cm::Recti &rect) {
				outline_string_centered(canvas, {0, 0, 0}, txt, rect);
				draw_string_centered(canvas, {255, 255, 255}, txt, rect);
			};

			{
				auto cpu_map = cartridge->report_cpu_mapping();
				constexpr auto sys_prg_size = nesem::bank_32k;
				auto prgsize = size(cartridge->rom().prg_rom);

				for (auto &&bank : cpu_map)
				{
					auto mem_multiplier = sys_prg_size / bank.size;
					auto mem_bank_rect = cpu_rom_area;
					mem_bank_rect.h /= mem_multiplier;
					mem_bank_rect.y += mem_bank_rect.h * ((bank.addr - 0x8000) / bank.size);

					auto rom_multiplier = prgsize / bank.size;
					auto rom_bank_rect = prg_rom_area;
					rom_bank_rect.h = int(rom_bank_rect.h / rom_multiplier);
					rom_bank_rect.y += rom_bank_rect.h * bank.bank;

					canvas.fill_rect(prg_bank_color(bank.bank), mem_bank_rect);
					canvas.draw_rect(prg_bank_color(bank.bank, 0.25f), mem_bank_rect);
					auto bank_txt = fmt::format("{}", bank.bank);
					draw_string_centered_shaded(canvas, bank_txt, mem_bank_rect);

					mem_bank_rect.h = 16;
					auto addr_txt = fmt::format("${:04X}", bank.addr);
					draw_string_centered_shaded(canvas, addr_txt, mem_bank_rect);

					canvas.fill_rect(prg_bank_color(bank.bank), rom_bank_rect);
					canvas.draw_rect(prg_bank_color(bank.bank, 0.25f), rom_bank_rect);
					draw_string_centered_shaded(canvas, bank_txt, rom_bank_rect);
				}
			}

			{
				auto ppu_map = cartridge->report_ppu_mapping();
				constexpr auto ppu_chr_size = nesem::bank_8k;
				auto chrsize = cartridge->chr_size();

				for (auto &&bank : ppu_map)
				{
					auto mem_multiplier = ppu_chr_size / bank.size;
					auto mem_bank_rect = ppu_chr_area;
					mem_bank_rect.h /= mem_multiplier;
					mem_bank_rect.y += mem_bank_rect.h * (bank.addr / bank.size);

					auto rom_multiplier = chrsize / bank.size;
					auto rom_bank_rect = rom_chr_area;
					rom_bank_rect.h = int(rom_bank_rect.h / rom_multiplier);
					rom_bank_rect.y += rom_bank_rect.h * bank.bank;

					canvas.fill_rect(chr_bank_color(bank.bank), mem_bank_rect);
					canvas.draw_rect(chr_bank_color(bank.bank, 0.25f), mem_bank_rect);

					canvas.fill_rect(chr_bank_color(bank.bank), rom_bank_rect);
					canvas.draw_rect(chr_bank_color(bank.bank, 0.25f), rom_bank_rect);

					auto bank_txt = fmt::format("{}", bank.bank);
					draw_string_centered_shaded(canvas, bank_txt, mem_bank_rect);

					mem_bank_rect.h = 16;

					auto addr_txt = fmt::format("${:04X}", bank.addr);
					draw_string_centered_shaded(canvas, addr_txt, mem_bank_rect);

					draw_string_centered_shaded(canvas, bank_txt, rom_bank_rect);
				}
			}

			auto cpu_bottom_area = cpu_rom_area;
			cpu_bottom_area.y = cpu_bottom_area.y + cpu_bottom_area.h - 16;
			cpu_bottom_area.h = 16;
			draw_string_centered_shaded(canvas, "$FFFF", cpu_bottom_area);

			auto ppu_bottom_area = ppu_chr_area;
			ppu_bottom_area.y = ppu_bottom_area.y + ppu_bottom_area.h - 16;
			ppu_bottom_area.h = 16;
			draw_string_centered_shaded(canvas, "$1FFF", ppu_bottom_area);
		}
	}

	void SideBar::draw_ppu_info(const nesem::Nes &nes, nesem::U8 current_palette, const ColorPalette &colors)
	{
		auto [canvas, lock] = texture.lock();
		canvas.fill({22, 22, 22});

		draw_string(canvas, {255, 255, 255}, fmt::format("scanline: {:>3}    cycle: {:>3}", nes.ppu().current_scanline(), nes.ppu().current_cycle()), {2, 2});

		auto pattern_tables = std::array{
			nes.ppu().read_pattern_table(0),
			nes.ppu().read_pattern_table(1),
		};

		auto palettes = nes.ppu().read_palettes();

		for (size_t index = 0; index < size(pattern_tables); ++index)
		{
			{
				auto fn = [&](const cm::Point2i pos) {
					auto palette_entry = pattern_tables[index].read_pixel(static_cast<nesem::U16>(pos.x), static_cast<nesem::U16>(pos.y), current_palette);
					auto color_index = palettes[palette_entry];

					return colors.color_at_index(color_index);
				};

				auto [pt_canvas, pt_lock] = nes_pattern_textures[index].lock();
				pt_canvas.update_points(std::cref(fn));
			}

			auto pos = cm::Point2{128 * static_cast<int>(index), 0};
			nes_pattern_pos[index] = {area.x + pos.x, area.h - 240 * 2 - 128};
		}

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Draw palettes
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		const auto palette_start_pos = cm::Point2{2, nes_pattern_pos[0].y - 4 - 16 * 2};
		auto palette_pos = palette_start_pos;
		auto color_size = cm::Size{14, 14};
		auto palette_size = cm::Size{color_size.w * 4 + 6, color_size.w + 4};

		for (nesem::U16 p = 0; p < 8; ++p)
		{
			for (int i = 0; i < 4; ++i)
			{
				auto color_pos = palette_pos + cm::Point2{3, 1};
				color_pos.x += (color_size.w) * i;
				auto color_rect = rect(color_pos, color_size);

				auto color_index = palettes[p * 4 + i];
				auto color = colors.color_at_index(color_index);

				canvas.fill_rect(color, color_rect);
				canvas.draw_rect({255, 255, 255}, color_rect);
			}

			if (p == current_palette)
			{
				auto selected_pos = palette_pos + cm::Point2{3, 1};
				auto selected_size = cm::Sizei{color_size.w * 4, color_size.h};

				canvas.draw_rect({255, 196, 128}, rect(selected_pos, selected_size));
				canvas.draw_rect({255, 128, 64}, rect(selected_pos - 1, selected_size + 2));
				canvas.draw_rect({255, 196, 128}, rect(selected_pos - 2, selected_size + 4));
			}
			if (((p + 1) % 4) == 0)
			{
				palette_pos.x = palette_start_pos.x;
				palette_pos.y += palette_size.h;
			}
			else
				palette_pos.x += palette_size.w;
		}

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		auto pos = cm::Point2{2, nes_pattern_pos[0].y + 128 + 4};
		auto offset = cm::Point2{16 * 8, 0};

		draw_string(canvas, {255, 255, 255}, "OEM memory - (x y) index attrib", pos);
		pos.y += 4;

		const auto &oam = nes.ppu().get_oam();
		for (size_t i = 0, end = std::size(oam); i < end; i += 4)
		{
			auto col = int((i / 4) % 2);
			if (col == 0)
				pos.y += 10;

			draw_string(canvas, {255, 255, 255}, fmt::format("({:>3} {:>3}) {:02X} {:02X}", oam[i + 3], oam[i + 0], oam[i + 1], oam[i + 2]), pos + offset * col);
		}

		pos.y += 20;

		draw_string(canvas, {255, 255, 255}, "Active sprites for scanline", pos);
		pos.y += 4;

		int index = 0;
		for (const auto &s : nes.ppu().get_active_sprites())
		{
			auto col = index++ % 2;
			if (col == 0)
				pos.y += 10;

			draw_string(canvas, {255, 255, 255}, fmt::format("({:>3} {:>3}) {:02X} {:02X}", s.x, s.y, s.index, s.attrib), pos + offset * col);
		}
	}

	void SideBar::draw_ppu_visualizer(const nesem::Nes &nes, const ColorPalette &colors)
	{
		auto [canvas, lock] = texture.lock();
		canvas.fill({22, 22, 22});

		const auto text_offset = cm::Point2{2, 2};
		auto pos = cm::Point2{0, 0};

		draw_string(canvas, {255, 255, 255}, "Sprites", pos + text_offset);
		pos.y += 12;

		auto pattern_tables = std::array{
			nes.ppu().read_pattern_table(0),
			nes.ppu().read_pattern_table(1),
		};

		auto palettes = nes.ppu().read_palettes();

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Render Sprites
		{
			auto [sprite_canvas, sprite_lock] = nes_sprite_texture.lock();

			// read the current transparent color
			// might not be technically correct in some situations, but should be good enough?
			auto clear_color = colors.color_at_index(palettes[0]);
			sprite_canvas.fill(clear_color);

			const auto &oam = nes.ppu().get_oam();
			const auto ppuctrl = nes.ppu().peek_ppuctrl();
			size_t pattern_index = (ppuctrl & 0b0000'1000) > 0 ? 1u : 0u;
			auto is_8x16 = (ppuctrl & 0b0010'0000) > 0;

			for (size_t i = 0, end = std::size(oam); i < end; i += 4)
			{
				auto sprite_x = oam[i + 3];

				// the OAM value resents the scanline the sprite is prepared on, it is rendered on on the following scanline
				auto sprite_y = oam[i + 0] + 1;

				// skip offscreen sprites

				// sprites can't clip off the right edge
				if (sprite_x > 255 - 8)
					continue;

				// skip offscreen sprites
				if (sprite_y >= 240)
					continue;

				auto index = oam[i + 1];
				auto attrib = oam[i + 2];

				if (is_8x16)
				{
					pattern_index = index & 1;
					index >>= 1;
				}

				const bool flip_x = attrib & 0b0100'0000;
				const bool flip_y = attrib & 0b1000'0000;

				nesem::U16 height = is_8x16 ? 16 : 8;

				if (is_8x16 && flip_y)
					++index;

				// draw the sprite
				for (nesem::U16 pixel_y = 0; pixel_y < height; ++pixel_y)
				{
					if (pixel_y == 8)
						index += flip_y ? -1 : 1;

					for (nesem::U16 pixel_x = 0; pixel_x < 8; ++pixel_x)
					{
						nesem::U16 pt_offset_x = flip_x ? 7 - pixel_x : pixel_x;
						nesem::U16 pt_offset_y = flip_y ? 7 - (pixel_y & 7) : (pixel_y & 7);

						auto palette = pattern_tables[pattern_index].read_pixel((index & 0xF) * 8 + pt_offset_x, ((index >> 4) & 0xF) * 8 + pt_offset_y, 4 | (attrib & 3));

						// check for transparent
						if ((palette & 3) == 0)
							continue;

						auto color_index = palettes[palette];
						auto color = colors.color_at_index(color_index);
						sprite_canvas.draw_point(color, {sprite_x + pixel_x, sprite_y + pixel_y});
					}
				}
			}
		}

		canvas.draw_rect({255, 255, 255}, cm::rect(pos, cm::Size{255, 240}));
		nes_sprite_pos = pos;
		pos.y += 240;

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		for (auto &&nt : std::array{0, 3})
		{
			auto index = nt & 1;

			draw_string(canvas, {255, 255, 255}, fmt::format("Nametable #{}", index), pos + text_offset);
			pos.y += 12;

			{
				auto name_table = nes.ppu().read_name_table(nt, pattern_tables);
				auto fn = [&](const cm::Point2i point) {
					auto color_index = name_table.read_pixel(static_cast<nesem::U16>(point.x), static_cast<nesem::U16>(point.y));
					return colors.color_at_index(color_index);
				};

				auto [nt_canvas, nt_lock] = nes_nametable_textures[index].lock();
				nt_canvas.update_points(std::cref(fn));
			}

			canvas.draw_rect({255, 255, 255}, cm::rect(pos, cm::Size{255, 240}));
			nes_nametable_pos[index] = pos;
			pos.y += 240;
		}
	}

	void SideBar::render(ui::Renderer &renderer, DebugMode mode)
	{
		if (mode == DebugMode::none)
			return;

		const auto pos = top_left(area);
		renderer.blit(pos, texture);

		switch (mode)
		{
		case DebugMode::none:
			// this is impossible since we bail early if in this mode, but this satisfies warnings about not all cases handled
			std::unreachable();
			break;

		case DebugMode::cpu_info:
			// no additional work needed
			break;

		case DebugMode::fg_info:
			for (size_t i = 0; i < size(nes_pattern_textures); ++i)
				renderer.blit(nes_pattern_pos[i], nes_pattern_textures[i]);
			break;

		case DebugMode::bg_info:
			renderer.blit(pos + nes_sprite_pos, nes_sprite_texture);

			for (size_t i = 0; i < size(nes_nametable_textures); ++i)
				renderer.blit(pos + nes_nametable_pos[i], nes_nametable_textures[i]);
			break;
		}
	}
}
