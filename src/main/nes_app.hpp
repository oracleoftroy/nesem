#pragma once
#include <optional>
#include <string>
#include <string_view>

#include <nes.hpp>

#include "bottom_bar.hpp"
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
	};

	class NesApp
	{
	public:
		explicit NesApp(ui::App &app);

		cm::Color read_palette(nesem::U16 entry) noexcept;
		cm::Color color_at_index(int index) noexcept;
		nesem::U8 get_current_palette() const noexcept;

	private:
		void load_rom(std::string_view filepath);
		void trigger_break(bool enable);

		void on_error(std::string_view message);
		void on_change_debug_mode(DebugMode mode);
		void on_nes_pixel(int x, int y, int color_index) noexcept;
		void on_nes_frame_ready() noexcept;

		void tick(ui::App &app, ui::Renderer &canvas, double deltatime);
		void handle_input(ui::App &app);
		void update(double deltatime);
		void render(ui::Renderer &renderer);

		nesem::Buttons read_controller(ui::App &app);

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

		BottomBar bottom_bar;
		SideBar side_bar;
		NesOverlay overlay;
		ControllerOverlay controller_overlay;
	};
}
