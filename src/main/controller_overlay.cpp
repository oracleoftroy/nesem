#include "controller_overlay.hpp"

#include <ui/app.hpp>
#include <ui/renderer.hpp>

namespace app
{
	ControllerOverlay::ControllerOverlay(ui::App &app, cm::Recti area)
		: texture(app.create_texture(cm::Size{66, 32})),
		  area(area)
	{
		texture.enable_blending(true);
	}

	void ControllerOverlay::update(util::Flags<nesem::Buttons> buttons)
	{
		using enum nesem::Buttons;

		if (buttons == last_buttons)
			return;

		last_buttons = buttons;

		static constexpr uint8_t alpha = 128 + 64;
		static constexpr auto outline_color = cm::Color{250, 253, 243, alpha};
		static constexpr auto bg_color = cm::Color{22, 22, 22, alpha};
		static constexpr auto inactive_color = cm::Color{105, 105, 105, alpha};
		static constexpr auto active_color = cm::Color{250, 30, 15, alpha};

		constexpr auto select_color = [](auto button_down) {
			if (button_down)
				return active_color;

			return inactive_color;
		};

		auto [canvas, lock] = texture.lock();

		auto canvas_area = rect({0, 0}, canvas.size());
		auto inner = cm::widen(canvas_area, -3);

		auto dpad_size = cm::Size{6, 6};
		auto dpad_offset = cm::Point2{5, 7};

		auto dpad_center = rect(dpad_offset + cm::Point2{dpad_size.w, dpad_size.h}, dpad_size);
		auto dpad_left = dpad_center - cm::Point2{6, 0};
		auto dpad_right = dpad_center + cm::Point2{6, 0};
		auto dpad_up = dpad_center - cm::Point2{0, 6};
		auto dpad_down = dpad_center + cm::Point2{0, 6};

		auto select_start_size = cm::Size{6, 4};
		auto select_area = rect({26, 14}, select_start_size);
		auto start_area = rect({35, 14}, select_start_size);

		const auto button_radius = 3;
		auto b_button_area = cm::Circlei(button_radius, {47, 16});
		auto a_button_area = cm::Circlei(button_radius, {56, 16});

		canvas.fill(outline_color);
		canvas.fill_rect(bg_color, inner);

		// dpad

		canvas.fill_rect(inactive_color, dpad_center);
		canvas.fill_rect(select_color(buttons.is_set(Left)), dpad_left);
		canvas.fill_rect(select_color(buttons.is_set(Up)), dpad_up);
		canvas.fill_rect(select_color(buttons.is_set(Right)), dpad_right);
		canvas.fill_rect(select_color(buttons.is_set(Down)), dpad_down);

		// outline the dpad
		canvas.draw_line(outline_color, bottom_left(dpad_up), top_left(dpad_up));
		canvas.draw_line(outline_color, top_left(dpad_up), top_right(dpad_up));
		canvas.draw_line(outline_color, top_right(dpad_up), bottom_right(dpad_up));

		canvas.draw_line(outline_color, top_left(dpad_right), top_right(dpad_right));
		canvas.draw_line(outline_color, top_right(dpad_right), bottom_right(dpad_right));
		canvas.draw_line(outline_color, bottom_left(dpad_right), bottom_right(dpad_right));

		canvas.draw_line(outline_color, top_right(dpad_down), bottom_right(dpad_down));
		canvas.draw_line(outline_color, bottom_left(dpad_down), bottom_right(dpad_down));
		canvas.draw_line(outline_color, bottom_left(dpad_down), top_left(dpad_down));

		canvas.draw_line(outline_color, bottom_left(dpad_left), bottom_right(dpad_left));
		canvas.draw_line(outline_color, bottom_left(dpad_left), top_left(dpad_left));
		canvas.draw_line(outline_color, top_left(dpad_left), top_right(dpad_left));

		// select and start buttons

		canvas.fill_rect(select_color(buttons.is_set(Select)), select_area);
		canvas.fill_rect(select_color(buttons.is_set(Start)), start_area);
		canvas.draw_rect(outline_color, select_area);
		canvas.draw_rect(outline_color, start_area);

		// a and b buttons

		canvas.fill_circle(select_color(buttons.is_set(B)), b_button_area);
		canvas.fill_circle(select_color(buttons.is_set(A)), a_button_area);
		canvas.draw_circle(outline_color, b_button_area);
		canvas.draw_circle(outline_color, a_button_area);
	}

	void ControllerOverlay::render(ui::Renderer &renderer)
	{
		auto size = texture.size();
		renderer.blit(bottom_right(area) - cm::Point2{size.w, size.h}, texture);
	}
}
