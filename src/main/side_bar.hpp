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
	class NesApp;
	enum class DebugMode;

	class SideBar
	{
	public:
		explicit SideBar() = default;
		explicit SideBar(ui::App &app, cm::Recti area);

		void update(DebugMode mode, nesem::Nes &nes, NesApp &app);
		void render(ui::Renderer &renderer, DebugMode mode);

	private:
		ui::Texture texture;
		std::array<ui::Texture, 2> nes_pattern_textures;
		std::array<cm::Point2i, 2> nes_pattern_pos;

		std::array<ui::Texture, 2> nes_nametable_textures;
		std::array<cm::Point2i, 2> nes_nametable_pos;

		cm::Recti area;
	};
}