#pragma once

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include <nes.hpp>

#include "bottom_bar.hpp"
#include "color_palette.hpp"
#include "controller_overlay.hpp"
#include "nes_overlay.hpp"
#include "side_bar.hpp"

#include <cm/math.hpp>
#include <ui/app.hpp>
#include <ui/texture.hpp>
#include <util/rng.hpp>

namespace app
{
	enum class DebugMode
	{
		none,
		bg_info,
		fg_info,
		cpu_info,
	};

	constexpr auto nes_resolution = cm::Size{256, 240};

	struct Config;

	class NesApp
	{
	public:
		explicit NesApp(ui::App &app, const Config &config);

		Config get_config() const noexcept;

	private:
		void on_file_drop(ui::App &app, std::string_view filename);
		void load_rom(ui::App &app, const std::filesystem::path &filepath);
		void load_pal(const std::filesystem::path &filepath);
		void trigger_break(ui::App &app, bool enable);

		void on_error(ui::App &app, std::string_view message);
		void on_change_debug_mode(DebugMode mode);
		void on_change_current_palette(nesem::U8 palette);
		void on_nes_pixel(int x, int y, nesem::U8 color_index, util::Flags<nesem::NesColorEmphasis> emphasis) noexcept;
		void on_nes_frame_ready() noexcept;

		void tick(ui::App &app, ui::Renderer &canvas, double deltatime);
		void handle_input(ui::App &app);
		void update(double deltatime);
		void render(ui::Renderer &renderer);

		void draw_screen();

		nesem::U8 read_controller(const ui::App &app);
		nesem::U8 read_zapper(const ui::App &app);
		bool sense_light(cm::Point2i pos) noexcept;

		std::filesystem::path data_path;

		ui::Key button_a;
		ui::Key button_b;
		ui::Key button_turbo_a;
		ui::Key button_turbo_b;
		ui::Key button_select;
		ui::Key button_start;
		ui::Key button_up;
		ui::Key button_down;
		ui::Key button_left;
		ui::Key button_right;

		// zapper state

		// number of frames remaining for the zapper to be in triggered state.
		int triggered_frame_counter = -1;

		int nes_scale = 3;
		int turbo_frame_cycle = 16;

		std::array<nesem::U16, nes_resolution.w * nes_resolution.h> nes_screen;
		ui::Texture nes_screen_texture;

		DebugMode debug_mode = DebugMode::none;
		ui::Key debug_mode_none;
		ui::Key debug_mode_bg;
		ui::Key debug_mode_fg;
		ui::Key debug_mode_cpu;

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

		BottomBar bottom_bar;
		SideBar side_bar;
		NesOverlay overlay;
		ControllerOverlay controller_overlay;

		ColorPalette colors = ColorPalette::default_palette();
	};
}
