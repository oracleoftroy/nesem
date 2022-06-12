#pragma once

#include <optional>
#include <string>

#include <cm/math.hpp>
#include <ui/texture.hpp>

namespace ui
{
	class App;
	class Renderer;
}

namespace app
{
	class NesOverlay
	{
	public:
		explicit NesOverlay() = default;
		explicit NesOverlay(ui::App &app, cm::Recti area, int scale);

		void hide();
		void show(const cm::Color &color, std::string_view msg);
		void render(ui::Renderer &renderer);

	private:
		ui::Texture texture;
		cm::Recti area;
		int scale;

		bool show_overlay = false;
	};
}
