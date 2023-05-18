#pragma once

#include <cm/math.hpp>

namespace ui
{
	class Texture;

	class Renderer final
	{
	public:
		explicit Renderer(void *renderer) noexcept;

		cm::Sizei size() const noexcept;

		void enable_blending(bool enable) noexcept;

		void fill(cm::Color color) noexcept;
		void draw_line(cm::Color color, cm::Point2i p1, cm::Point2i p2) noexcept;
		void draw_rect(cm::Color color, cm::Recti rect) noexcept;

		void fill_rect(cm::Color color, cm::Recti rect) noexcept;

		void blit(cm::Point2i dst, const Texture &texture, const std::optional<cm::Recti> &src_rect = std::nullopt, cm::Sizei scale = {1, 1}) noexcept;

		// this class is just used as an interface. Users shouldn't try to store it
		Renderer(const Renderer &) = delete;
		Renderer(Renderer &&) = delete;
		Renderer &operator=(const Renderer &) = delete;
		Renderer &operator=(Renderer &&) = delete;

	private:
		void *renderer = nullptr;
	};
}
