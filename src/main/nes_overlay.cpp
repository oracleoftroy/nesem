#include "nes_overlay.hpp"

#include <fmt/format.h>

#include "text.hpp"

#include <ui/app.hpp>
#include <ui/renderer.hpp>

namespace app
{
	NesOverlay::NesOverlay(ui::App &app, cm::Recti area, int scale)
		: texture(app.create_texture(size(area))), area(area), scale(scale)
	{
		texture.enable_blending(true);
	}

	void NesOverlay::hide()
	{
		show_overlay = false;
	}

	void NesOverlay::show(const cm::Color &color, std::string_view msg)
	{
		show_overlay = true;
		auto [nes_overlay, lock] = texture.lock();

		auto size = nes_overlay.size();

		auto msg_width = int(msg.size() * 8);

		auto string_pos = cm::Point2{size.w / 2 - (msg_width / 2), size.h / 2 - 4};

		auto overlay_area = rect(string_pos, cm::Sizei{msg_width, 8});
		overlay_area.x -= 8;
		overlay_area.y -= 8;
		overlay_area.w += 16;
		overlay_area.h += 16;

		nes_overlay.fill(color);
		nes_overlay.fill_rect({24, 24, 24, 240}, overlay_area);
		nes_overlay.draw_rect({255, 255, 255}, overlay_area);
		draw_string(nes_overlay, {255, 255, 255}, msg, string_pos);
	}

	void NesOverlay::render(ui::Renderer &renderer)
	{
		if (show_overlay)
			renderer.blit(top_left(area), texture, std::nullopt, {scale, scale});
	}
}
