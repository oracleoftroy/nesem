#pragma once

#include <array>

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

		void update(DebugMode mode, const nesem::Nes &nes, NesApp &app, const ColorPalette &colors);
		void render(ui::Renderer &renderer, DebugMode mode);

	private:
		void draw_cpu_info(const nesem::Nes &nes);
		void draw_ppu_info(DebugMode mode, const nesem::Nes &nes, NesApp &app, const ColorPalette &colors);

	private:
		ui::Texture texture;
		std::array<ui::Texture, 2> nes_pattern_textures;
		std::array<cm::Point2i, 2> nes_pattern_pos;

		std::array<ui::Texture, 2> nes_nametable_textures;
		std::array<cm::Point2i, 2> nes_nametable_pos;

		cm::Recti area;
	};
}
