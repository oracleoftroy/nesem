#include <SDL2/SDL_render.h>

#include <ui/renderer.hpp>
#include <ui/texture.hpp>
#include <util/logging.hpp>

namespace
{
	constexpr SDL_Renderer *r(void *p) noexcept
	{
		return static_cast<SDL_Renderer *>(p);
	}

	SDL_Texture *t(const ui::Texture &tex) noexcept
	{
		return static_cast<SDL_Texture *>(tex.impl());
	}

	constexpr SDL_Rect sdl(cm::Recti r) noexcept
	{
		return SDL_Rect{
			.x = r.x,
			.y = r.y,
			.w = r.w,
			.h = r.h,
		};
	}
}

namespace ui
{
	Renderer::Renderer(void *renderer) noexcept
		: renderer(renderer)
	{
	}

	void Renderer::present() const noexcept
	{
		SDL_RenderPresent(r(renderer));
	}

	cm::Sizei Renderer::size() const noexcept
	{
		cm::Sizei result;
		SDL_RenderGetLogicalSize(r(renderer), &result.w, &result.h);

		return result;
	}

	void Renderer::enable_blending(bool enable) noexcept
	{
		// TODO: consider other blend modes?
		SDL_BlendMode blend_mode = enable ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE;
		SDL_SetRenderDrawBlendMode(r(renderer), blend_mode);
	}

	void Renderer::fill(cm::Color color) noexcept
	{
		if (SDL_SetRenderDrawColor(r(renderer), color.r, color.g, color.b, color.a) != 0)
			LOG_WARN("Error setting renderer draw color: {}", SDL_GetError());

		if (SDL_RenderClear(r(renderer)) != 0)
			LOG_WARN("Error clearing renderer: {}", SDL_GetError());
	}

	void Renderer::draw_line(cm::Color color, cm::Point2i p1, cm::Point2i p2) noexcept
	{
		if (SDL_SetRenderDrawColor(r(renderer), color.r, color.g, color.b, color.a) != 0)
			LOG_WARN("Error setting renderer draw color: {}", SDL_GetError());

		if (SDL_RenderDrawLine(r(renderer), p1.x, p1.y, p2.x, p2.y) != 0)
			LOG_WARN("Error drawing line: {}", SDL_GetError());
	}

	void Renderer::draw_rect(cm::Color color, cm::Recti rect) noexcept
	{
		if (SDL_SetRenderDrawColor(r(renderer), color.r, color.g, color.b, color.a) != 0)
			LOG_WARN("Error setting renderer draw color: {}", SDL_GetError());

		auto sdlrect = sdl(rect);
		if (SDL_RenderDrawRect(r(renderer), &sdlrect) != 0)
			LOG_WARN("Error drawing rect: {}", SDL_GetError());
	}

	void Renderer::fill_rect(cm::Color color, cm::Recti rect) noexcept
	{
		if (SDL_SetRenderDrawColor(r(renderer), color.r, color.g, color.b, color.a) != 0)
			LOG_WARN("Error setting renderer draw color: {}", SDL_GetError());

		auto sdlrect = sdl(rect);
		if (SDL_RenderFillRect(r(renderer), &sdlrect) != 0)
			LOG_WARN("Error filling rect: {}", SDL_GetError());
	}

	void Renderer::blit(cm::Point2i dst, const Texture &texture, const std::optional<cm::Recti> &src_rect, cm::Sizei scale) noexcept
	{
		auto make_src = [&] {
			if (src_rect)
				return *src_rect;

			return cm::rect({0, 0}, texture.size());
		};

		auto src = make_src();
		auto dest = cm::rect(dst, cm::size(src) * scale);

		auto sdlsrc = sdl(src);
		auto sdldest = sdl(dest);
		SDL_RenderCopy(r(renderer), t(texture), &sdlsrc, &sdldest);
	}
}
