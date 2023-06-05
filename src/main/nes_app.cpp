#include "nes_app.hpp"

#include <filesystem>

#include "config.hpp"
#include "text.hpp"

#include <ui/app.hpp>
#include <ui/renderer.hpp>
#include <ui/texture.hpp>
#include <util/logging.hpp>

namespace app
{
	NesApp::NesApp(const Config &config)
		: app{ui::App::create("NES emulator", cm::Size{1024, 768})},
		  nes({
			  .error = std::bind_front(&NesApp::on_error, this),
			  .draw = std::bind_front(&NesApp::on_nes_pixel, this),
			  .frame_ready = std::bind_front(&NesApp::on_nes_frame_ready, this),
			  .player1 = std::make_unique<nesem::NesController>(std::bind_front(&NesApp::read_controller, this)),
			  .player2 = std::make_unique<nesem::NesInputDevice>(std::bind_front(&NesApp::read_zapper, this)),
			  .nes20db_filename = config.nes20db_filename.value_or(std::filesystem::path{}),
			  .user_data_dir = ui::App::get_user_data_path("nesem"),
		  })
	{
		{ // setup callbacks
			app.on_file_drop = std::bind_front(&NesApp::on_file_drop, this);
		}

		{ // setup keys
			button_a = ui::App::key_from_name(config.controller_1.a.c_str());
			button_b = ui::App::key_from_name(config.controller_1.b.c_str());
			button_turbo_a = ui::App::key_from_name(config.controller_1.turbo_a.c_str());
			button_turbo_b = ui::App::key_from_name(config.controller_1.turbo_b.c_str());

			button_select = ui::App::key_from_name(config.controller_1.select.c_str());
			button_start = ui::App::key_from_name(config.controller_1.start.c_str());
			button_up = ui::App::key_from_name(config.controller_1.up.c_str());
			button_down = ui::App::key_from_name(config.controller_1.down.c_str());
			button_left = ui::App::key_from_name(config.controller_1.left.c_str());
			button_right = ui::App::key_from_name(config.controller_1.right.c_str());

			escape_key = ui::App::key_from_name("Escape");

			toggle_fullscreen_key = ui::App::key_from_name("Return");
			palette_next_key = ui::App::key_from_name("]");
			palette_prev_key = ui::App::key_from_name("[");

			debug_mode_none = ui::App::key_from_name("0");
			debug_mode_bg = ui::App::key_from_name("1");
			debug_mode_fg = ui::App::key_from_name("2");
			debug_mode_cpu = ui::App::key_from_name("3");

			break_key = ui::App::key_from_name("Pause");
			run_key = ui::App::key_from_name("F5");

			step_cpu_instruction_key = ui::App::key_from_name("F8");
			step_ppu_cycle_key = ui::App::key_from_name("F9");
			step_ppu_scanline_key = ui::App::key_from_name("F10");
			step_ppu_frame_key = ui::App::key_from_name("F11");

			reset_key = ui::App::key_from_name("R");
		}

		{ // setup screen areas
			// pre-measure rects for the different areas of the screen
			auto size = app.renderer_size();
			auto nes_area = rect({0, 0}, nes_resolution * nes_scale);

			auto side_bar_size = cm::Size{size.w - nes_area.w, size.h};
			auto side_area = rect({nes_area.w, 0}, side_bar_size);

			auto bottom_bar_size = cm::Size{nes_area.w, size.h - nes_area.h};
			auto bottom_area = rect({0, nes_area.h}, bottom_bar_size);

			bottom_bar = BottomBar(app, bottom_area);
			side_bar = SideBar(app, side_area);
			overlay = NesOverlay(app, rect({0, 0}, nes_resolution), nes_scale);
			controller_overlay = ControllerOverlay(app, nes_area);
		}

		turbo_frame_cycle = config.controller_1.turbo_speed;

		nes_screen_texture = app.create_texture({256, 240});

		if (config.last_played_rom)
			load_rom(*config.last_played_rom);

		if (config.palette)
			load_pal(*config.palette);
	}

	Config NesApp::get_config() const noexcept
	{
		Config config;

		config.last_played_rom = rom_name;

		config.controller_1.turbo_speed = turbo_frame_cycle;
		config.controller_1.turbo_a = ui::App::name_from_key(button_turbo_a);
		config.controller_1.turbo_b = ui::App::name_from_key(button_turbo_b);
		config.controller_1.a = ui::App::name_from_key(button_a);
		config.controller_1.b = ui::App::name_from_key(button_b);
		config.controller_1.select = ui::App::name_from_key(button_select);
		config.controller_1.start = ui::App::name_from_key(button_start);
		config.controller_1.up = ui::App::name_from_key(button_up);
		config.controller_1.down = ui::App::name_from_key(button_down);
		config.controller_1.left = ui::App::name_from_key(button_left);
		config.controller_1.right = ui::App::name_from_key(button_right);

		return config;
	}

	void NesApp::on_file_drop(std::string_view filename)
	{
		auto path = std::filesystem::path(filename);

		if (path.extension() == ".nes")
			load_rom(path);
		else if (path.extension() == ".pal")
			load_pal(path);
		else
		{
			LOG_WARN("Unknown file type for '{}', trying to load as iNES", path.string());
			load_rom(path);
		}
	}

	void NesApp::load_rom(const std::filesystem::path &filepath)
	{
		rom_loaded = nes.load_rom(filepath);

		if (!rom_loaded)
		{
			rom_name = std::nullopt;
			overlay.show({100, 149, 237, 127}, "No ROM Loaded");
		}
		else
		{
			rom_name = filepath.string();
			trigger_break(false);
		}

		bottom_bar.update(system_break, rom_name);
	}

	void NesApp::load_pal(const std::filesystem::path &filepath)
	{
		auto new_colors = ColorPalette::from_file(filepath);
		if (new_colors)
			colors = std::move(*new_colors);
		else
			LOG_WARN("Could not load color palette from '{}', keeping previous", filepath.string());
	}

	void NesApp::trigger_break(bool enable)
	{
		ui::App::enable_screensaver(enable);

		system_break = enable;
		step = nesem::NesClockStep::None;

		if (!system_break)
			overlay.hide();

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

		// the side bar is normally updated when we tick the nes. But we don't tick the nes when we are paused, so force an update
		if (system_break)
			side_bar.update(debug_mode, nes, current_palette, colors);
	}

	void NesApp::on_change_current_palette(nesem::U8 palette)
	{
		current_palette = palette;

		// the side bar is normally updated when we tick the nes. But we don't tick the nes when we are paused, so force an update
		if (system_break)
			side_bar.update(debug_mode, nes, current_palette, colors);
	}

	bool NesApp::tick()
	{
		auto real_deltatime = clock.update().count();
		app.update_fps(real_deltatime);

		// if we have a large frameskip, cap it
		auto delta_time = std::min(real_deltatime, 0.25);

		if (!app.process_events())
			return false;

		handle_input();
		update(delta_time);
		render();

		return true;
	}

	void NesApp::handle_input()
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

		if (app.key_pressed(debug_mode_cpu))
		{
			on_change_debug_mode(DebugMode::cpu_info);
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
			on_change_current_palette(nesem::U8(current_palette + 1) % 8);
			LOG_INFO("palette {} selected", current_palette);
		}

		if (app.key_pressed(palette_prev_key))
		{
			on_change_current_palette(nesem::U8(current_palette - 1) % 8);
			LOG_INFO("palette {} selected", current_palette);
		}

		if (app.key_pressed(reset_key) && app.modifiers(ui::KeyMods::ctrl))
		{
			LOG_INFO("resetting NES...");
			nes.reset();
		}
	}

	void NesApp::update(double delta_time)
	{
		if (rom_loaded)
		{
			using enum nesem::NesClockStep;

			if (!system_break)
			{
				nes.tick(delta_time);
				side_bar.update(debug_mode, nes, current_palette, colors);
			}
			else if (step != None)
			{
				nes.step(step);
				side_bar.update(debug_mode, nes, current_palette, colors);
				step = None;

				draw_screen();
			}
		}
	}

	void NesApp::render()
	{
		auto renderer = app.renderer();

		renderer.fill({22, 22, 22});
		renderer.blit({0, 0}, nes_screen_texture, std::nullopt, {nes_scale, nes_scale});

		side_bar.render(renderer, debug_mode);
		bottom_bar.render(renderer);
		overlay.render(renderer);
		controller_overlay.render(renderer);

		renderer.present();
	}

	void NesApp::on_nes_pixel(int x, int y, nesem::U8 color_index, util::Flags<nesem::NesColorEmphasis> emphasis) noexcept
	{
		nes_screen[y * nes_resolution.w + x] = to_color_index(color_index, emphasis);
	}

	void NesApp::on_nes_frame_ready() noexcept
	{
		// update zapper info
		if (triggered_frame_counter > 0)
			--triggered_frame_counter;

		// not that break is unlikely per se, but when we are running at full speed, we want this to be as fast as possible
		if (system_break) [[unlikely]]
			return;

		draw_screen();
	}

	void NesApp::draw_screen()
	{
		auto [canvas, lock] = nes_screen_texture.lock();

		std::transform(begin(nes_screen), end(nes_screen), canvas.ptr(),
			[this, format = canvas.format()](nesem::U16 index) {
				return to_pixel(format, colors.color_at_index(index));
			});
	}

	nesem::U8 NesApp::read_controller()
	{
		using enum nesem::Buttons;
		util::Flags<nesem::Buttons> result = None;

		bool in_turbo = std::cmp_greater_equal(nes.ppu().current_frame() % turbo_frame_cycle, turbo_frame_cycle / 2);

		if (in_turbo && app.key_down(button_turbo_a))
			result.set(A);
		if (in_turbo && app.key_down(button_turbo_b))
			result.set(B);

		if (app.key_down(button_a))
			result.set(A);
		if (app.key_down(button_b))
			result.set(B);
		if (app.key_down(button_select))
			result.set(Select);
		if (app.key_down(button_start))
			result.set(Start);
		if (app.key_down(button_up))
			result.set(Up);
		if (app.key_down(button_down))
			result.set(Down);
		if (app.key_down(button_left))
			result.set(Left);
		if (app.key_down(button_right))
			result.set(Right);

		// The physical NES controller can't have both up and down or both left and right, and weird things can happen in some games if both are set.
		// We will treat both buttons down as neither button down, as if the buttons cancel each other out

		if (result.is_set(Up, Down))
			result.clear(Up, Down);

		if (result.is_set(Left, Right))
			result.clear(Left, Right);

		controller_overlay.update(result);
		return result.raw_value();
	}

	nesem::U8 NesApp::read_zapper()
	{
		// output
		// bits xxx43xxx
		//         ^^--- light sensed (0 sensed, 1 not sensed)
		//         L---- trigger pulled (1 pulled, 0 released)
		nesem::U8 result = 0;

		bool mouse_down = app.mouse_down(1);

		// when trigger is pulled, hold trigger high for several frames
		// nesdev indicates that "most existing zapper games ... usually fire on a transition from 1 to 0."
		// also nesdev:
		// "The official Zapper's trigger returns 0 while the trigger is fully open, 1 while it is halfway in, and 0 again after it has been pulled all the way to where it goes clunk.
		// "The large capacitor (10µF) inside the Zapper when combined with the 10kΩ pullup inside the console means that it will take approximately 100ms to change to "released" after the trigger has been fully pulled."
		// 100ms gives about a 6 frame delay between the trigger going high and low again
		// Using a smaller delay for less input lag
		constexpr auto frame_delay = 2;

		if (triggered_frame_counter == 0 && !mouse_down)
			triggered_frame_counter = -1;
		else if (triggered_frame_counter == -1 && mouse_down)
			triggered_frame_counter = frame_delay;

		if (triggered_frame_counter > 0)
			result |= (1 << 4);

		// bit goes low if we detect light as the ppu is rendering around our mouse position
		if (auto pos = app.mouse_position() / nes_scale;
			!sense_light(pos))
			result |= (1 << 3);

		return result;
	}

	bool NesApp::sense_light(cm::Point2i pos) noexcept
	{
		// no light if cursor isn't in nes window
		if (pos.x < 0 || pos.x >= nes_resolution.w ||
			pos.y < 0 || pos.y >= nes_resolution.h)
			return false;

		auto cycle = nes.ppu().current_cycle();
		auto scanline = nes.ppu().current_scanline();

		// NesDev: "The sensor may turn on and off in 10 to 25 scanlines"
		// We will only sense light if the pixel at pos was recently rendered
		bool nearby = (pos.y < scanline || (pos.y == scanline && pos.x < cycle)) && (scanline - pos.y) <= 18;

		if (!nearby)
			return false;

		auto pixel = nes_screen[pos.y * nes_resolution.w + pos.x];

		// check if we read a "light" pixel
		return pixel == 0x20 ||
			pixel == 0x30 ||
			pixel == 0x31 ||
			pixel == 0x32 ||
			pixel == 0x33 ||
			pixel == 0x34 ||
			pixel == 0x35 ||
			pixel == 0x36 ||
			pixel == 0x37 ||
			pixel == 0x38 ||
			pixel == 0x39 ||
			pixel == 0x3a ||
			pixel == 0x3b ||
			pixel == 0x3c;
	}
}
