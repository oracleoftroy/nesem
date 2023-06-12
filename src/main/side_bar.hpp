#pragma once

#include <array>

#include <nes_types.hpp>

#include <cm/math.hpp>
#include <ui/texture.hpp>

namespace ui
{
	class App;
	class Renderer;
}

namespace nesem
{
	class Nes;
}

namespace app
{
	class ColorPalette;
	class NesApp;
	enum class DebugMode;

	class SideBar
	{
	public:
		explicit SideBar() = default;
		explicit SideBar(ui::App &app, cm::Recti area);

		void update(DebugMode mode, const nesem::Nes &nes, nesem::U8 current_palette, const ColorPalette &colors);
		void render(ui::Renderer &renderer, DebugMode mode);

	private:
		void draw_cpu_info(const nesem::Nes &nes);
		void draw_ppu_info(const nesem::Nes &nes, nesem::U8 current_palette, const ColorPalette &colors);
		void draw_ppu_visualizer(const nesem::Nes &nes, const ColorPalette &colors);

	private:
		ui::Texture texture;
		std::array<ui::Texture, 2> nes_pattern_textures;
		std::array<cm::Point2i, 2> nes_pattern_pos;

		std::array<ui::Texture, 2> nes_nametable_textures;
		std::array<cm::Point2i, 2> nes_nametable_pos;

		ui::Texture nes_sprite_texture;
		cm::Point2i nes_sprite_pos;

		cm::Recti area;
	};
}
