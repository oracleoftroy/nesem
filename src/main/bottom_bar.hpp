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
	class BottomBar
	{
	public:
		explicit BottomBar() = default;
		explicit BottomBar(ui::App &app, cm::Recti area);

		void update(bool in_break, const std::optional<std::string> &rom_name);
		void render(ui::Renderer &renderer);

	private:
		ui::Texture texture;
		cm::Recti area;
	};
}
