#pragma once

#include <concepts>
#include <cstdint>
#include <optional>

#include <cm/math.hpp>

namespace ui
{
	class Canvas final
	{
	public:
		Canvas() noexcept = default;
		explicit Canvas(cm::Sizei size, cm::ColorFormat format = {}) noexcept;
		explicit Canvas(cm::Sizei size, cm::ColorFormat format, uint32_t *ptr) noexcept;
		~Canvas();
		Canvas(Canvas &&other) noexcept;
		Canvas &operator=(Canvas &&other) noexcept;

		Canvas(const Canvas &other) noexcept = delete;
		Canvas &operator=(const Canvas &other) noexcept = delete;

		explicit operator bool() const noexcept;
		[[nodiscard]] cm::Sizei size() const noexcept;
		[[nodiscard]] cm::ColorFormat format() const noexcept;
		[[nodiscard]] uint32_t *ptr() const noexcept;

		void enable_blending(bool enable) noexcept;

		void fill(cm::Color color) noexcept;
		void draw_point(cm::Color color, cm::Point2i point) noexcept;
		void draw_line(cm::Color color, cm::Point2i p1, cm::Point2i p2, bool antialias = false) noexcept;
		void draw_triangle(cm::Color color, cm::Point2i p1, cm::Point2i p2, cm::Point2i p3, bool antialias = false) noexcept;
		void draw_rect(cm::Color color, cm::Recti rect) noexcept;
		void draw_curve(cm::Color color, cm::Point2i p1, cm::Point2i p2, cm::Point2i p3) noexcept;
		void draw_circle(cm::Color color, cm::Circlei circle) noexcept;

		void fill_triangle(cm::Color color, cm::Point2i p1, cm::Point2i p2, cm::Point2i p3) noexcept;
		void fill_rect(cm::Color color, cm::Recti rect) noexcept;
		void fill_circle(cm::Color color, cm::Circlei circle) noexcept;

		void blit(cm::Point2i dst, const Canvas &src, const std::optional<cm::Recti> &src_rect = std::nullopt, cm::Sizei scale = {1, 1}) noexcept;

		// avoid repeated bounds check
		void update_points(std::invocable<cm::Point2i> auto fn) noexcept;

	private:
		void cleanup() noexcept;

		[[nodiscard]] ptrdiff_t to_index(cm::Point2i point) const noexcept;
		[[nodiscard]] cm::Point2i from_index(ptrdiff_t index) const noexcept;

		void do_draw_point(uint32_t pixel, cm::Point2i point) noexcept;

	private:
		cm::Sizei canvas_size;
		cm::ColorFormat canvas_format = {};
		uint32_t *canvas_ptr = nullptr;

		bool owns_canvas = true;
		bool blending_enabled = false;
	};

	void Canvas::update_points(std::invocable<cm::Point2i> auto fn) noexcept
	{
		for (int y = 0; y < canvas_size.h; ++y)
		{
			for (int x = 0; x < canvas_size.w; ++x)
			{
				const auto pos = cm::Point2{x, y};
				const auto color = fn(pos);
				do_draw_point(cm::to_pixel(canvas_format, color), pos);
			}
		}
	}
}
