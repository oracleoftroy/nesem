#include <filesystem>

#include <ui/app.hpp>
#include <util/logging.hpp>
#include <util/rng.hpp>

#include "nes.hpp"
#include "text.hpp"

enum class DebugMode
{
	none,
	bg_info,
	fg_info,
};

std::filesystem::path find_file(const std::filesystem::path &path)
{
	namespace fs = std::filesystem;

	if (!path.has_filename())
	{
		LOG_WARN("Path does not name a file: '{}'", path.string());
		return path;
	}

	// remember the bit that varies from our current working directory
	const auto relative = fs::proximate(path);
	auto dir = fs::current_path();

	while (!fs::exists(dir / relative))
	{
		LOG_INFO("Trying {}", (dir / relative).string());

		auto previous = std::exchange(dir, dir.parent_path());

		// we are at the root directory and can't go up anymore, so bail
		if (dir == previous)
			break;
	}

	auto p = dir / relative;

	// we found a path to the file, return it
	if (fs::exists(p))
	{
		LOG_INFO("Found {}", p.string());
		return p;
	}

	// could not find path, return original path
	LOG_INFO("Could not find file {}", path.string());
	return path;
}

class NesApp
{
public:
	NesApp(ui::App &app)
		: nes(std::bind_front(&NesApp::on_nes_pixel, this), std::bind_front(&NesApp::read_controller, this, std::ref(app)))
	{
		app.on_update = std::bind_front(&NesApp::tick, this);
		app.on_file_drop = std::bind_front(&NesApp::load_rom, this);

		button_a = app.key_from_name("/");
		button_b = app.key_from_name(".");
		button_select = app.key_from_name(",");
		button_start = app.key_from_name("Space");
		button_up = app.key_from_name("W");
		button_down = app.key_from_name("S");
		button_left = app.key_from_name("A");
		button_right = app.key_from_name("D");

		toggle_fullscreen_key = app.key_from_name("F11");
		palette_next_key = app.key_from_name("]");
		palette_prev_key = app.key_from_name("[");

		debug_mode_none = app.key_from_name("0");
		debug_mode_bg = app.key_from_name("1");
		debug_mode_fg = app.key_from_name("2");

		rom_loaded = nes.load_rom(find_file(R"(data/nestest.nes)"));
	}

private:
	void load_rom(std::string_view filepath)
	{
		rom_loaded = nes.load_rom(filepath);
	}

	void tick(ui::App &app, ui::Canvas &canvas, float deltatime)
	{
		handle_input(app);
		update(deltatime);
		render(canvas);
	}

	void handle_input(ui::App &app)
	{
		if (app.key_pressed(toggle_fullscreen_key))
		{
			fullscreen = !fullscreen;
			LOG_INFO("fullscreen now: {}", fullscreen);
			app.fullscreen(fullscreen);
		}

		if (app.key_pressed(debug_mode_bg))
		{
			debug_mode = DebugMode::bg_info;
			LOG_INFO("Debug mode now: {}", int(debug_mode));
		}

		if (app.key_pressed(debug_mode_fg))
		{
			debug_mode = DebugMode::fg_info;
			LOG_INFO("Debug mode now: {}", int(debug_mode));
		}

		if (app.key_pressed(debug_mode_none))
		{
			debug_mode = DebugMode::none;
			LOG_INFO("Debug mode now: {}", int(debug_mode));
		}

		if (app.key_pressed(palette_next_key))
			current_palette = nesem::U8(current_palette + 1) % 8;
		if (app.key_pressed(palette_prev_key))
			current_palette = nesem::U8(current_palette - 1) % 8;
	}

	void update(double deltatime)
	{
		if (rom_loaded)
		{
			// nes.tick(deltatime);
			nes.step(nesem::NesClockStep::OneFrame);
		}
	}

	void render(ui::Canvas &canvas)
	{
		canvas.fill({});

		auto size = nes_screen.size();
		auto scale = 3;

		canvas.blit({0, 0}, nes_screen, std::nullopt, {scale, scale});

		if (debug_mode == DebugMode::fg_info || debug_mode == DebugMode::bg_info)
		{
			auto pattern_0_pos = cm::Point2{canvas.size().w - (nes_pattern_1.size().w * 2), 0};
			auto pattern_1_pos = cm::Point2{canvas.size().w - (nes_pattern_1.size().w * 1), 0};
			auto nametable_1_pos = cm::Point2{canvas.size().w - nes_nametable_1.size().w, canvas.size().h - nes_nametable_1.size().h};
			auto nametable_0_pos = cm::Point2{nametable_1_pos.x, nametable_1_pos.y - nes_nametable_0.size().h};

			nes.ppu().draw_pattern_table(0, current_palette, std::bind_front(&NesApp::on_pattern_pixel, this, 0));
			canvas.blit(pattern_0_pos, nes_pattern_0, std::nullopt);

			nes.ppu().draw_pattern_table(1, current_palette, std::bind_front(&NesApp::on_pattern_pixel, this, 1));
			canvas.blit(pattern_1_pos, nes_pattern_1, std::nullopt);

			canvas.draw_line({200, 200, 200}, {size.w * scale, 0}, {size.w * scale, canvas.size().h});
			canvas.draw_rect({200, 200, 200}, rect(pattern_0_pos, nes_pattern_0.size()));
			canvas.draw_rect({200, 200, 200}, rect(pattern_1_pos, nes_pattern_1.size()));

			auto palette_end_pos = draw_palettes(canvas);

			if (debug_mode == DebugMode::bg_info)
			{
				auto [fine_x, fine_y, coarse_x, coarse_y, nt] = nes.ppu().get_scroll_info();

				auto pos = cm::Point2{nes_screen.size().w * scale + 2, palette_end_pos.y};
				draw_string(canvas, {255, 255, 255}, fmt::format("fine x,y: {}, {}", fine_x, fine_y), pos);
				pos.y += 8;
				draw_string(canvas, {255, 255, 255}, fmt::format("coarse x,y: {:>2}, {:>2}", coarse_x, coarse_y), pos);
				pos.y += 8;
				draw_string(canvas, {255, 255, 255}, fmt::format("nametable: {}", nt), pos);

				nes.ppu().draw_name_table(0, std::bind_front(&NesApp::on_nametable_pixel, this, 0));
				canvas.blit(nametable_0_pos, nes_nametable_0, std::nullopt);
				nes.ppu().draw_name_table(1, std::bind_front(&NesApp::on_nametable_pixel, this, 1));
				canvas.blit(nametable_1_pos, nes_nametable_1, std::nullopt);
				canvas.draw_rect({200, 200, 200}, rect(nametable_0_pos, nes_nametable_0.size()));
				canvas.draw_rect({200, 200, 200}, rect(nametable_1_pos, nes_nametable_1.size()));
			}

			if (debug_mode == DebugMode::fg_info)
			{
				auto pos = cm::Point2{nes_screen.size().w * scale + 2, palette_end_pos.y};
				const auto palette_start_pos = cm::Point2{canvas.size().w - 256 + 2, 128 + 2};

				const auto &oam = nes.ppu().get_oam();
				for (size_t i = 0, end = std::size(oam); i < end; i += 4)
				{
					draw_string(canvas, {255, 255, 255}, fmt::format("({:>3} {:>3}) {:02X} {:02X}", oam[i + 3], oam[i + 0], oam[i + 1], oam[i + 2]), pos);
					pos.y += 8;
				}
			}
		}
	}

	cm::Point2i draw_palettes(ui::Canvas &canvas) noexcept
	{
		const auto palette_start_pos = cm::Point2{canvas.size().w - 256 + 2, 128 + 2};
		auto palette_pos = palette_start_pos;
		auto color_size = cm::Size{14, 14};
		auto palette_size = cm::Size{color_size.w * 4 + 6, color_size.w + 4};

		nesem::U16 palette_base_addr = 0x3F00;
		for (nesem::U16 p = 0; p < 8; ++p)
		{
			for (int i = 0; i < 4; ++i)
			{
				auto color_pos = palette_pos + cm::Point2{3, 1};
				color_pos.x += (color_size.w) * i;
				auto color_rect = rect(color_pos, color_size);

				auto color_index = nes.ppu().read(palette_base_addr + p * 4 + i);

				canvas.fill_rect(nes_colors[color_index], color_rect);
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

		return palette_pos;
	}

	void on_nes_pixel(int x, int y, int color_index) noexcept
	{
		nes_screen.draw_point(nes_colors[color_index], {x, y});
	}

	void on_pattern_pixel(int index, int x, int y, int color_index) noexcept
	{
		(index == 0 ? nes_pattern_0 : nes_pattern_1).draw_point(nes_colors[color_index], {x, y});
	}

	void on_nametable_pixel(int index, int x, int y, int color_index) noexcept
	{
		(index == 0 ? nes_nametable_0 : nes_nametable_1).draw_point(nes_colors[color_index], {x, y});
	}

	nesem::Buttons read_controller(ui::App &app)
	{
		using enum nesem::Buttons;
		auto result = None;

		if (app.key_down(button_a))
			result |= A;
		if (app.key_down(button_b))
			result |= B;
		if (app.key_down(button_select))
			result |= Select;
		if (app.key_down(button_start))
			result |= Start;
		if (app.key_down(button_up))
			result |= Up;
		if (app.key_down(button_down))
			result |= Down;
		if (app.key_down(button_left))
			result |= Left;
		if (app.key_down(button_right))
			result |= Right;

		return result;
	}

	ui::Key button_a;
	ui::Key button_b;
	ui::Key button_select;
	ui::Key button_start;
	ui::Key button_up;
	ui::Key button_down;
	ui::Key button_left;
	ui::Key button_right;

	std::array<cm::Color, 64> nes_colors{
		cm::Color{84,  84,  84 },
		cm::Color{0,   30,  116},
		cm::Color{8,   16,  144},
		cm::Color{48,  0,   136},
		cm::Color{68,  0,   100},
		cm::Color{92,  0,   48 },
		cm::Color{84,  4,   0  },
		cm::Color{60,  24,  0  },
		cm::Color{32,  42,  0  },
		cm::Color{8,   58,  0  },
		cm::Color{0,   64,  0  },
		cm::Color{0,   60,  0  },
		cm::Color{0,   50,  60 },
		cm::Color{0,   0,   0  },
		cm::Color{0,   0,   0  },
		cm::Color{0,   0,   0  },
		cm::Color{152, 150, 152},
		cm::Color{8,   76,  196},
		cm::Color{48,  50,  236},
		cm::Color{92,  30,  228},
		cm::Color{136, 20,  176},
		cm::Color{160, 20,  100},
		cm::Color{152, 34,  32 },
		cm::Color{120, 60,  0  },
		cm::Color{84,  90,  0  },
		cm::Color{40,  114, 0  },
		cm::Color{8,   124, 0  },
		cm::Color{0,   118, 40 },
		cm::Color{0,   102, 120},
		cm::Color{0,   0,   0  },
		cm::Color{0,   0,   0  },
		cm::Color{0,   0,   0  },
		cm::Color{236, 238, 236},
		cm::Color{76,  154, 236},
		cm::Color{120, 124, 236},
		cm::Color{176, 98,  236},
		cm::Color{228, 84,  236},
		cm::Color{236, 88,  180},
		cm::Color{236, 106, 100},
		cm::Color{212, 136, 32 },
		cm::Color{160, 170, 0  },
		cm::Color{116, 196, 0  },
		cm::Color{76,  208, 32 },
		cm::Color{56,  204, 108},
		cm::Color{56,  180, 204},
		cm::Color{60,  60,  60 },
		cm::Color{0,   0,   0  },
		cm::Color{0,   0,   0  },
		cm::Color{236, 238, 236},
		cm::Color{168, 204, 236},
		cm::Color{188, 188, 236},
		cm::Color{212, 178, 236},
		cm::Color{236, 174, 236},
		cm::Color{236, 174, 212},
		cm::Color{236, 180, 176},
		cm::Color{228, 196, 144},
		cm::Color{204, 210, 120},
		cm::Color{180, 222, 120},
		cm::Color{168, 226, 144},
		cm::Color{152, 226, 180},
		cm::Color{160, 214, 228},
		cm::Color{160, 162, 160},
		cm::Color{0,   0,   0  },
		cm::Color{0,   0,   0  },
	};

	ui::Canvas nes_screen = ui::Canvas({256, 240});
	ui::Canvas nes_pattern_0 = ui::Canvas({128, 128});
	ui::Canvas nes_pattern_1 = ui::Canvas({128, 128});
	ui::Canvas nes_nametable_0 = ui::Canvas({256, 240});
	ui::Canvas nes_nametable_1 = ui::Canvas({256, 240});

	DebugMode debug_mode = DebugMode::none;
	ui::Key debug_mode_none;
	ui::Key debug_mode_bg;
	ui::Key debug_mode_fg;

	ui::Key toggle_fullscreen_key;
	bool fullscreen = false;

	ui::Key palette_next_key;
	ui::Key palette_prev_key;

	bool rom_loaded = false;
	nesem::Nes nes;
	util::Random rng;

	nesem::U8 current_palette = 0;
};

int main(int argc, char *argv[])
{
	LOG_INFO("Starting: {}", fmt::join(argv, argv + argc, " "));
	LOG_INFO("Working directory: {}", std::filesystem::current_path().string());

	constexpr auto window_size = cm::Size{1024, 768};

	auto app = ui::App::create("NES emulator", window_size);

	if (!app)
	{
		LOG_ERROR("Error creating app, exiting...");
		return -1;
	}

	auto core = NesApp{app};

	app.run();

	LOG_INFO("Exiting...");
	return 0;
}
