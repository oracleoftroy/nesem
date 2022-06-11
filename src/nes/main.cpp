#include <filesystem>
#include <optional>

#include "nes.hpp"
#include "text.hpp"

#include <ui/app.hpp>
#include <ui/renderer.hpp>
#include <ui/texture.hpp>
#include <util/logging.hpp>
#include <util/rng.hpp>

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
		: nes({
			  .error = std::bind_front(&NesApp::on_error, this),
			  .draw = std::bind_front(&NesApp::on_nes_pixel, this),
			  .frame_ready = std::bind_front(&NesApp::on_nes_frame_ready, this),
			  .player1 = std::bind_front(&NesApp::read_controller, this, std::ref(app)),
			  .nes20db_filename = find_file(R"(data/nes20db.xml)"),
		  })
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

		escape_key = app.key_from_name("Escape");

		toggle_fullscreen_key = app.key_from_name("Return");
		palette_next_key = app.key_from_name("]");
		palette_prev_key = app.key_from_name("[");

		debug_mode_none = app.key_from_name("0");
		debug_mode_bg = app.key_from_name("1");
		debug_mode_fg = app.key_from_name("2");

		break_key = app.key_from_name("Pause");
		run_key = app.key_from_name("F5");

		step_cpu_instruction_key = app.key_from_name("F8");
		step_ppu_cycle_key = app.key_from_name("F9");
		step_ppu_scanline_key = app.key_from_name("F10");
		step_ppu_frame_key = app.key_from_name("F11");

		reset_key = app.key_from_name("R");

		load_rom(find_file(R"(data/nestest.nes)").string());

		nes_screen_texture = app.create_texture({256, 240});
		nes_pending_texture = app.create_texture({256, 240});
		nes_overlay_texture = app.create_texture({256, 240});
		nes_overlay_texture.enable_blending(true);

		nes_pattern_0_texture = app.create_texture({128, 128});
		nes_pattern_1_texture = app.create_texture({128, 128});

		nes_nametable_0_texture = app.create_texture({256, 240});
		nes_nametable_1_texture = app.create_texture({256, 240});

		text_overlay_texture = app.create_texture(app.renderer_size());
		text_overlay_texture.enable_blending(true);

		nes_screen = nes_pending_texture.unsafe_lock();
	}

private:
	void load_rom(std::string_view filepath)
	{
		error_msg.clear();
		rom_loaded = nes.load_rom(filepath);

		if (!rom_loaded)
			rom_name = std::nullopt;
		else
			rom_name = filepath;
	}

	void trigger_break(bool enable)
	{
		system_break = enable;
		step = nesem::NesClockStep::None;

		if (system_break)
		{
			// we take control of the screen when stepping
			nes_pending_texture.unsafe_unlock();
			std::swap(nes_pending_texture, nes_screen_texture);
			nes_screen = std::nullopt;
		}
		else
		{
			// restore normal frame handling
			nes_screen = nes_pending_texture.unsafe_lock();
		}
	}

	void on_error(std::string_view message)
	{
		error_msg = message;
		trigger_break(true);
	}

	void tick(ui::App &app, ui::Renderer &canvas, double deltatime)
	{
		handle_input(app);
		update(deltatime);
		render(canvas);
	}

	void handle_input(ui::App &app)
	{
		if (app.key_pressed(escape_key))
		{
			fullscreen = false;
			LOG_INFO("fullscreen now: {}", fullscreen);
			app.fullscreen(fullscreen);
		}

		if (app.key_pressed(toggle_fullscreen_key) && app.modifiers(ui::KeyMods::alt))
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

		if (app.key_pressed(run_key))
		{
			trigger_break(false);
			LOG_INFO("System break now: {}", system_break);
		}

		if (app.key_pressed(break_key))
		{
			trigger_break(!system_break);
			LOG_INFO("System break now: {}", system_break);
		}

		if (system_break)
		{
			using enum nesem::NesClockStep;

			if (app.key_pressed(step_cpu_instruction_key))
			{
				step = OneCpuCycle;
				LOG_INFO("Step one CPU instruction");
			}

			if (app.key_pressed(step_ppu_cycle_key))
			{
				step = OnePpuCycle;
				LOG_INFO("Step one PPU instruction");
			}

			if (app.key_pressed(step_ppu_scanline_key))
			{
				step = OnePpuScanline;
				LOG_INFO("Step one PPU scanline");
			}

			if (app.key_pressed(step_ppu_frame_key))
			{
				step = OneFrame;
				LOG_INFO("Step one PPU frame");
			}
		}

		if (app.key_pressed(palette_next_key))
		{
			current_palette = nesem::U8(current_palette + 1) % 8;
			LOG_INFO("palette {} selected", current_palette);
		}

		if (app.key_pressed(palette_prev_key))
		{
			current_palette = nesem::U8(current_palette - 1) % 8;
			LOG_INFO("palette {} selected", current_palette);
		}

		if (app.key_pressed(reset_key) && app.modifiers(ui::KeyMods::ctrl))
		{
			LOG_INFO("resetting NES...");
			error_msg.clear();
			nes.reset();
		}
	}

	void update([[maybe_unused]] double deltatime)
	{
		if (rom_loaded)
		{
			using enum nesem::NesClockStep;

			if (!system_break)
				nes.tick(deltatime);
			else
			{
				auto [screen, lock] = nes_screen_texture.lock();
				nes_screen = std::move(screen);

				if (step != None)
				{
					nes.step(step);
					step = None;
				}

				nes_screen = std::nullopt;
			}
		}
	}

	void render(ui::Renderer &renderer)
	{
		renderer.fill({});

		auto size = nes_screen_texture.size();
		auto canvas_size = renderer.size();

		renderer.blit({0, 0}, nes_screen_texture, std::nullopt, {nes_scale, nes_scale});

		if (!rom_loaded)
		{
			draw_overlay_text(renderer, {100, 149, 237, 127}, "No ROM Loaded");
		}
		else if (!error_msg.empty())
		{
			draw_overlay_text(renderer, {0, 0, 0, 127}, error_msg);
		}
		else if (system_break)
		{
			draw_overlay_text(renderer, {0, 0, 0, 127}, "Paused");
		}

		{
			auto [text_canvas, text_lock] = text_overlay_texture.lock();
			text_canvas.fill({0, 0, 0, 0});

			if (debug_mode == DebugMode::fg_info || debug_mode == DebugMode::bg_info)
			{
				auto pattern_0_pos = cm::Point2{canvas_size.w - (nes_pattern_1_texture.size().w * 2), 0};
				auto pattern_1_pos = cm::Point2{canvas_size.w - (nes_pattern_1_texture.size().w * 1), 0};
				auto nametable_1_pos = cm::Point2{canvas_size.w - nes_nametable_1_texture.size().w, canvas_size.h - nes_nametable_1_texture.size().h};
				auto nametable_0_pos = cm::Point2{nametable_1_pos.x, nametable_1_pos.y - nes_nametable_0_texture.size().h};

				auto pattern_tables = std::array{
					nes.ppu().read_pattern_table(0),
					nes.ppu().read_pattern_table(1),
				};

				{
					auto [canvas, lock] = nes_pattern_0_texture.lock();
					for (int y = 0; y < 128; ++y)
						for (int x = 0; x < 128; ++x)
						{
							auto palette_entry = pattern_tables[0].read_pixel(x, y, current_palette);
							auto color_index = read_palette(palette_entry);
							canvas.draw_point(nes_colors[color_index], {x, y});
						}
				}
				{
					auto [canvas, lock] = nes_pattern_1_texture.lock();
					for (int y = 0; y < 128; ++y)
						for (int x = 0; x < 128; ++x)
						{
							auto palette_entry = pattern_tables[1].read_pixel(x, y, current_palette);
							auto color_index = read_palette(palette_entry);
							canvas.draw_point(nes_colors[color_index], {x, y});
						}
				}

				renderer.blit(pattern_0_pos, nes_pattern_0_texture);
				renderer.blit(pattern_1_pos, nes_pattern_1_texture);

				renderer.draw_line({200, 200, 200}, {size.w * nes_scale, 0}, {size.w * nes_scale, canvas_size.h});
				renderer.draw_rect({200, 200, 200}, rect(pattern_0_pos, nes_pattern_0_texture.size()));
				renderer.draw_rect({200, 200, 200}, rect(pattern_1_pos, nes_pattern_1_texture.size()));

				auto palette_end_pos = draw_palettes(renderer);

				if (debug_mode == DebugMode::bg_info)
				{
					auto [fine_x, fine_y, coarse_x, coarse_y, nt] = nes.ppu().get_scroll_info();

					auto pos = cm::Point2{nes_screen_texture.size().w * nes_scale + 2, palette_end_pos.y};

					draw_string(text_canvas, {255, 255, 255}, fmt::format("fine x,y: {}, {}", fine_x, fine_y), pos);
					pos.y += 8;
					draw_string(text_canvas, {255, 255, 255}, fmt::format("coarse x,y: {:>2}, {:>2}", coarse_x, coarse_y), pos);
					pos.y += 8;
					draw_string(text_canvas, {255, 255, 255}, fmt::format("nametable: {}", nt), pos);

					auto name_tables = std::array{
						nes.ppu().read_name_table(0, pattern_tables),
						// nes.ppu().read_name_table(1, pattern_tables),
					    // nes.ppu().read_name_table(2, pattern_tables),
						nes.ppu().read_name_table(3, pattern_tables),
					};

					{
						auto [canvas, lock] = nes_nametable_0_texture.lock();
						for (int y = 0; y < 240; ++y)
							for (int x = 0; x < 256; ++x)
							{
								auto color_index = name_tables[0].read_pixel(x, y);
								canvas.draw_point(nes_colors[color_index], {x, y});
							}
					}
					{
						auto [canvas, lock] = nes_nametable_1_texture.lock();
						for (int y = 0; y < 240; ++y)
							for (int x = 0; x < 256; ++x)
							{
								auto color_index = name_tables[1].read_pixel(x, y);
								canvas.draw_point(nes_colors[color_index], {x, y});
							}
					}

					renderer.blit(nametable_0_pos, nes_nametable_0_texture);
					renderer.blit(nametable_1_pos, nes_nametable_1_texture);
					renderer.draw_rect({200, 200, 200}, rect(nametable_0_pos, nes_nametable_0_texture.size()));
					renderer.draw_rect({200, 200, 200}, rect(nametable_1_pos, nes_nametable_1_texture.size()));
				}

				if (debug_mode == DebugMode::fg_info)
				{
					auto pos = cm::Point2{nes_screen_texture.size().w * nes_scale + 2, palette_end_pos.y};
					auto offset = cm::Point2{16 * 8, 0};

					const auto palette_start_pos = cm::Point2{canvas_size.w - 256 + 2, 128 + 2};

					draw_string(text_canvas, {255, 255, 255}, "OEM memory - (x y) index attrib", pos);
					pos.y += 2;

					const auto &oam = nes.ppu().get_oam();
					for (size_t i = 0, end = std::size(oam); i < end; i += 4)
					{
						auto col = int((i / 4) % 2);
						if (col == 0)
							pos.y += 10;

						draw_string(text_canvas, {255, 255, 255}, fmt::format("({:>3} {:>3}) {:02X} {:02X}", oam[i + 3], oam[i + 0], oam[i + 1], oam[i + 2]), pos + offset * col);
					}

					// pos = cm::Point2{nes_screen_texture.size().w * nes_scale + 2 + 16 * 8, palette_end_pos.y};
					pos.y += 20;

					draw_string(text_canvas, {255, 255, 255}, "Active sprites for scanline", pos);
					pos.y += 2;

					int index = 0;
					for (const auto &s : nes.ppu().get_active_sprites())
					{
						auto col = int(index++ % 2);
						if (col == 0)
							pos.y += 10;

						draw_string(text_canvas, {255, 255, 255}, fmt::format("({:>3} {:>3}) {:02X} {:02X}", s.x, s.y, s.index, s.attrib), pos + offset * col);
					}
				}
			}

			{
				auto pos = cm::Point2{0, canvas_size.h - 10};
				draw_string(text_canvas, {255, 255, 255}, fmt::format("{}", rom_name.value_or("No rom loaded")), pos);

				pos.y -= 12;
				draw_string(text_canvas, {255, 255, 255}, "Debug info:   off: 0    background info: 1    foreground info: 2", pos);

				pos.y -= 12;
				if (system_break)
					draw_string(text_canvas, {255, 255, 255}, "F5: resume   F8: step cpu   F9: step PPU cycle   F10: step scanline   F11: step frame", pos);
				else
					draw_string(text_canvas, {255, 255, 255}, "Move: WASD   A: '/'   B: '.'   Start: spacebar   Select: ','      Break key to pause emulation", pos);
			}
		}
		renderer.blit({0, 0}, text_overlay_texture);
	}

	void draw_overlay_text(ui::Renderer &canvas, const cm::Color &color, std::string_view msg) noexcept
	{
		{
			auto [nes_overlay, lock] = nes_overlay_texture.lock();
			auto size = nes_overlay.size();

			auto msg_width = int(msg.size() * 8);

			auto string_pos = cm::Point2{size.w / 2 - (msg_width / 2), size.h / 2 - 4};

			auto overlay_area = rect(string_pos, cm::Sizei{msg_width, 8});
			overlay_area.x -= 8;
			overlay_area.y -= 8;
			overlay_area.w += 16;
			overlay_area.h += 16;

			nes_overlay.fill(color);
			nes_overlay.fill_rect({24, 24, 24, 240}, overlay_area);
			nes_overlay.draw_rect({255, 255, 255}, overlay_area);
			draw_string(nes_overlay, {255, 255, 255}, msg, string_pos);
		}

		canvas.blit({0, 0}, nes_overlay_texture, std::nullopt, {nes_scale, nes_scale});
	}

	nesem::U8 read_palette(nesem::U16 entry) noexcept
	{
		nesem::U16 palette_base_addr = 0x3F00;
		return nes.ppu().read(palette_base_addr + entry);
	}

	cm::Point2i draw_palettes(ui::Renderer &canvas) noexcept
	{
		const auto palette_start_pos = cm::Point2{canvas.size().w - 256 + 2, 128 + 2};
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

				auto color_index = read_palette(p * 4 + i);

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
		if (!nes_screen) [[unlikely]]
		{
			LOG_WARN("nes_screen not locked!");
			return;
		}

		nes_screen->draw_point(nes_colors[color_index], {x, y});
	}

	void on_nes_frame_ready() noexcept
	{
		// not that break is unlikely per se, but when we are running at full speed, we want this to be as fast as possible
		if (system_break) [[unlikely]]
			return;

		if (!nes_screen) [[unlikely]]
		{
			LOG_WARN("nes_screen not locked!");
			return;
		}

		nes_pending_texture.unsafe_unlock();
		std::swap(nes_pending_texture, nes_screen_texture);
		nes_screen = nes_pending_texture.unsafe_lock();
	}

	void on_pattern_pixel(ui::Canvas &canvas, int x, int y, int color_index) noexcept
	{
		canvas.draw_point(nes_colors[color_index], {x, y});
	}

	void on_nametable_pixel(ui::Canvas &canvas, int x, int y, int color_index) noexcept
	{
		canvas.draw_point(nes_colors[color_index], {x, y});
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

		// The physical NES controller can't have both up and down or both left and right, and weird things can happen in some games if both are set.
		// We will treat both buttons down as neither button down, as if the buttons cancel each other out

		if ((result & (Up | Down)) == (Up | Down))
			result &= ~(Up | Down);

		if ((result & (Left | Right)) == (Left | Right))
			result &= ~(Left | Right);

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

	int nes_scale = 3;

	std::optional<ui::Canvas> nes_screen;

	ui::Texture nes_screen_texture;
	ui::Texture nes_pending_texture;
	ui::Texture nes_overlay_texture;
	ui::Texture nes_pattern_0_texture;
	ui::Texture nes_pattern_1_texture;
	ui::Texture nes_nametable_0_texture;
	ui::Texture nes_nametable_1_texture;

	ui::Texture text_overlay_texture;

	DebugMode debug_mode = DebugMode::none;
	ui::Key debug_mode_none;
	ui::Key debug_mode_bg;
	ui::Key debug_mode_fg;

	bool system_break = false;
	ui::Key break_key;
	ui::Key run_key;
	ui::Key reset_key;

	nesem::NesClockStep step = nesem::NesClockStep::None;

	ui::Key step_cpu_instruction_key;
	ui::Key step_ppu_cycle_key;
	ui::Key step_ppu_scanline_key;
	ui::Key step_ppu_frame_key;

	ui::Key escape_key;
	ui::Key toggle_fullscreen_key;
	bool fullscreen = false;

	ui::Key palette_next_key;
	ui::Key palette_prev_key;

	bool rom_loaded = false;
	std::optional<std::string> rom_name;

	nesem::Nes nes;
	util::Random rng;

	nesem::U8 current_palette = 0;
	std::string error_msg;
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
