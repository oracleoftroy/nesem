#include "nes_app.hpp"

#include <filesystem>

#include "text.hpp"

#include <ui/app.hpp>
#include <ui/renderer.hpp>
#include <ui/texture.hpp>
#include <util/logging.hpp>

namespace
{
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
}

namespace app
{
	NesApp::NesApp(ui::App &app)
		: nes({
			  .error = std::bind_front(&NesApp::on_error, this),
			  .draw = std::bind_front(&NesApp::on_nes_pixel, this),
			  .frame_ready = std::bind_front(&NesApp::on_nes_frame_ready, this),
			  .player1 = std::bind_front(&NesApp::read_controller, this, std::ref(app)),
			  .nes20db_filename = find_file(R"(data/nes20db.xml)"),
		  })
	{
		{ // setup callbacks
			app.on_update = std::bind_front(&NesApp::tick, this);
			app.on_file_drop = std::bind_front(&NesApp::load_rom, this);
		}

		{ // setup keys
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
		}

		{ // setup screen areas
			// pre-measure rects for the different areas of the screen
			auto size = app.renderer_size();
			auto nes_resolution = cm::Size{256, 240};
			auto nes_area = rect({0, 0}, nes_resolution * nes_scale);

			auto side_bar_size = cm::Size{size.w - nes_area.w, size.h};
			auto side_area = rect({nes_area.w, 0}, side_bar_size);

			auto bottom_bar_size = cm::Size{nes_area.w, size.h - nes_area.h};
			auto bottom_area = rect({0, nes_area.h}, bottom_bar_size);

			bottom_bar = BottomBar(app, bottom_area);
			side_bar = SideBar(app, side_area);
			overlay = NesOverlay(app, rect({0, 0}, nes_resolution), nes_scale);
		}

		// extra stuff that should mostly go away....?

		nes_screen_texture = app.create_texture({256, 240});
		nes_pending_texture = app.create_texture({256, 240});

		nes_screen = nes_pending_texture.unsafe_lock();

		load_rom(find_file(R"(data/nestest.nes)").string());
	}

	cm::Color NesApp::read_palette(nesem::U16 entry) noexcept
	{
		nesem::U16 palette_base_addr = 0x3F00;
		auto index = nes.ppu().read(palette_base_addr + entry);

		return nes_colors[index];
	}

	cm::Color NesApp::color_at_index(int index) noexcept
	{
		return nes_colors[index];
	}

	nesem::U8 NesApp::get_current_palette() const noexcept
	{
		return current_palette;
	}

	void NesApp::load_rom(std::string_view filepath)
	{
		rom_loaded = nes.load_rom(filepath);

		if (!rom_loaded)
		{
			rom_name = std::nullopt;
			overlay.show({100, 149, 237, 127}, "No ROM Loaded");
		}
		else
		{
			rom_name = filepath;
			trigger_break(false);
		}

		bottom_bar.update(system_break, rom_name);
	}

	void NesApp::trigger_break(bool enable)
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
			overlay.hide();
		}

		bottom_bar.update(system_break, rom_name);
	}

	void NesApp::on_error(std::string_view message)
	{
		overlay.show({0, 0, 0, 127}, message);
		trigger_break(true);
	}

	void NesApp::on_change_debug_mode(DebugMode mode)
	{
		debug_mode = mode;
		side_bar.update(debug_mode, nes, *this);
	}

	void NesApp::tick(ui::App &app, ui::Renderer &canvas, double deltatime)
	{
		handle_input(app);
		update(deltatime);
		render(canvas);
	}

	void NesApp::handle_input(ui::App &app)
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
			on_change_debug_mode(DebugMode::bg_info);
			LOG_INFO("Debug mode now: {}", int(debug_mode));
		}

		if (app.key_pressed(debug_mode_fg))
		{
			on_change_debug_mode(DebugMode::fg_info);
			LOG_INFO("Debug mode now: {}", int(debug_mode));
		}

		if (app.key_pressed(debug_mode_none))
		{
			on_change_debug_mode(DebugMode::none);
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

			if (system_break)
				overlay.show({0, 0, 0, 127}, "Paused");

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
			nes.reset();
		}
	}

	void NesApp::update([[maybe_unused]] double deltatime)
	{
		if (rom_loaded)
		{
			using enum nesem::NesClockStep;

			if (!system_break)
			{
				nes.tick(deltatime);
				side_bar.update(debug_mode, nes, *this);
			}
			else
			{
				auto [screen, lock] = nes_screen_texture.lock();
				nes_screen = std::move(screen);

				if (step != None)
				{
					nes.step(step);
					side_bar.update(debug_mode, nes, *this);
					step = None;
				}

				nes_screen = std::nullopt;
			}
		}
	}

	void NesApp::render(ui::Renderer &renderer)
	{
		// renderer.fill({});
		renderer.fill({22, 22, 22});

		auto size = nes_screen_texture.size();
		auto canvas_size = renderer.size();

		renderer.blit({0, 0}, nes_screen_texture, std::nullopt, {nes_scale, nes_scale});

		side_bar.render(renderer, debug_mode);
		bottom_bar.render(renderer);
		overlay.render(renderer);
	}

	void NesApp::on_nes_pixel(int x, int y, int color_index) noexcept
	{
		if (!nes_screen) [[unlikely]]
		{
			LOG_WARN("nes_screen not locked!");
			return;
		}

		nes_screen->draw_point(nes_colors[color_index], {x, y});
	}

	void NesApp::on_nes_frame_ready() noexcept
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

	nesem::Buttons NesApp::read_controller(ui::App &app)
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

}
