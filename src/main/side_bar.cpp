#include "side_bar.hpp"

#include <fmt/format.h>

#include "nes_app.hpp"
#include "text.hpp"

#include <ui/app.hpp>
#include <ui/renderer.hpp>

namespace app
{
	SideBar::SideBar(ui::App &app, cm::Recti area)
		: texture(app.create_texture(size(area))),
		  nes_pattern_textures{app.create_texture({128, 128}), app.create_texture({128, 128})},
		  nes_nametable_textures{app.create_texture({256, 240}), app.create_texture({256, 240})},
		  area(area)
	{
	}

	void SideBar::update(DebugMode mode, nesem::Nes &nes, NesApp &app)
	{
		if (mode == DebugMode::none)
			return;

		auto [canvas, lock] = texture.lock();
		canvas.fill({22, 22, 22});

		draw_string(canvas, {255, 255, 255}, fmt::format("scanline: {:>3}    cycle: {:>3}", nes.ppu().current_scanline(), nes.ppu().current_cycle()), {2, 2});

		auto pattern_tables = std::array{
			nes.ppu().read_pattern_table(0),
			nes.ppu().read_pattern_table(1),
		};

		auto current_palette = app.get_current_palette();

		for (size_t index = 0; index < size(pattern_tables); ++index)
		{
			{
				auto [pt_canvas, pt_lock] = nes_pattern_textures[index].lock();
				for (int y = 0; y < 128; ++y)
				{
					for (int x = 0; x < 128; ++x)
					{
						auto palette_entry = pattern_tables[index].read_pixel(x, y, current_palette);
						auto color = app.read_palette(palette_entry);
						pt_canvas.draw_point(color, {x, y});
					}
				}
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

				auto color = app.read_palette(p * 4 + i);

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

		switch (mode)
		{
		case DebugMode::bg_info:
		{
			for (auto &&nt : std::array{0, 3})
			{
				auto index = nt & 1;
				auto name_table = nes.ppu().read_name_table(nt, pattern_tables);

				{
					auto [nt_canvas, nt_lock] = nes_nametable_textures[index].lock();
					for (int y = 0; y < 240; ++y)
					{
						for (int x = 0; x < 256; ++x)
						{
							auto color_index = name_table.read_pixel(x, y);
							nt_canvas.draw_point(app.color_at_index(color_index), {x, y});
						}
					}
				}

				auto pos = cm::Point2{0, area.h - (240 * (2 - index))};
				nes_nametable_pos[index] = {area.x, pos.y};
			}

			auto [fine_x, fine_y, coarse_x, coarse_y, nt] = nes.ppu().get_scroll_info();

			auto pos = cm::Point2{2, palette_start_pos.y - 12};

			draw_string(canvas, {255, 255, 255}, fmt::format("nametable: {}", nt), pos);
			pos.y -= 10;
			draw_string(canvas, {255, 255, 255}, fmt::format("coarse x,y: {:>2}, {:>2}", coarse_x, coarse_y), pos);
			pos.y -= 10;
			draw_string(canvas, {255, 255, 255}, fmt::format("fine x,y: {}, {}", fine_x, fine_y), pos);
		}
		break;

		case DebugMode::fg_info:
		{
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
		break;

		case DebugMode::none:
			// nothing to do, here so that if we add a new mode, we can get a warning for unhandled cases
			break;
		}
	}

	void SideBar::render(ui::Renderer &renderer, DebugMode mode)
	{
		if (mode != DebugMode::none)
		{
			renderer.blit(top_left(area), texture);
			for (size_t i = 0; i < size(nes_pattern_textures); ++i)
				renderer.blit(nes_pattern_pos[i], nes_pattern_textures[i]);

			if (mode == DebugMode::bg_info)
			{
				for (size_t i = 0; i < size(nes_nametable_textures); ++i)
					renderer.blit(nes_nametable_pos[i], nes_nametable_textures[i]);
			}
		}
	}
}
