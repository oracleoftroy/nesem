#include "bottom_bar.hpp"

#include <fmt/format.h>

#include "text.hpp"

#include <ui/app.hpp>
#include <ui/renderer.hpp>

namespace app
{
	BottomBar::BottomBar(ui::App &app, cm::Recti area)
		: texture(app.create_texture(size(area))), area(area)
	{
	}

	void BottomBar::update(bool in_break, const std::optional<std::string> &rom_name)
	{
		auto [canvas, lock] = texture.lock();

		// canvas.fill({});
		canvas.fill({22, 22, 22});

		auto pos = cm::Point2{0, area.h - 10};
		draw_string(canvas, {255, 255, 255}, fmt::format("{}", rom_name.value_or("No rom loaded")), pos);

		pos.y -= 12;
		draw_string(canvas, {255, 255, 255}, "Debug info:   off: 0    background info: 1    foreground info: 2    CPU and memory info: 3", pos);

		pos.y -= 12;
		if (in_break)
			draw_string(canvas, {255, 255, 255}, "F5: resume   F8: step cpu   F9: step PPU cycle   F10: step scanline   F11: step frame", pos);
		else
			draw_string(canvas, {255, 255, 255}, "Move: WASD   A: '/'   B: '.'   Start: spacebar   Select: ','      Break key to pause emulation", pos);
	}

	void BottomBar::render(ui::Renderer &renderer)
	{
		renderer.blit(top_left(area), texture);
	}
}
