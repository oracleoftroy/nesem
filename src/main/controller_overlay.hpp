#pragma once

#include <nes_types.hpp>

#include <cm/math.hpp>
#include <ui/texture.hpp>

namespace ui
{
	class App;
	class Renderer;
}

namespace app
{
	class NesApp;

	class ControllerOverlay
	{
	public:
		explicit ControllerOverlay() = default;
		explicit ControllerOverlay(ui::App &app, cm::Recti area);

		void update(util::Flags<nesem::Buttons> buttons);
		void render(ui::Renderer &renderer);

	private:
		ui::Texture texture;
		cm::Recti area;
	};
}
