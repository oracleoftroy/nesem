#include <utility>

#include <SDL2/SDL_render.h>

#include <ui/canvas.hpp>
#include <ui/texture.hpp>
#include <util/logging.hpp>

namespace ui
{
	Texture::Texture(void *texture) noexcept
		: texture(texture)
	{
	}

	Texture::~Texture()
	{
		destroy();
	}

	Texture::Texture(Texture &&other) noexcept
		: texture(std::exchange(other.texture, nullptr))
	{
	}

	Texture &Texture::operator=(Texture &&other) noexcept
	{
		destroy();
		texture = std::exchange(other.texture, nullptr);
		return *this;
	}

	void Texture::enable_blending(bool enable) noexcept
	{
		auto t = static_cast<SDL_Texture *>(texture);

		// TODO: consider other blend modes?
		SDL_BlendMode blend_mode = enable ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE;
		SDL_SetTextureBlendMode(t, blend_mode);
	}

	Canvas Texture::lock() noexcept
	{
		auto t = static_cast<SDL_Texture *>(texture);
		Uint32 texture_format;
		int access;
		cm::Sizei size;

		if (SDL_QueryTexture(t, &texture_format, &access, &size.w, &size.h) != 0)
		{
			LOG_WARN("Could not query texture: {}", SDL_GetError());
			return {};
		}

		int bpp;
		Uint32 mask_r, mask_g, mask_b, mask_a;
		if (!SDL_PixelFormatEnumToMasks(texture_format, &bpp, &mask_r, &mask_g, &mask_b, &mask_a))
		{
			LOG_WARN("Could not get texture masks: {}", SDL_GetError());
			return {};
		}

		if (bpp != 32)
		{
			LOG_WARN("Unexpected bits per pixel value {}, expected 32", bpp);
			return {};
		}

		void *pixels = nullptr;
		int pitch = 0;
		if (SDL_LockTexture(t, nullptr, &pixels, &pitch) != 0)
		{
			LOG_WARN("Could not lock texture: {}", SDL_GetError());
			return {};
		}

		if (pitch != size.w * 4)
		{
			LOG_CRITICAL("Texture pitch is not the width of the texture, need to fix canvas!");
			return {};
		}

		auto format = cm::ColorFormat{
			.mask_r = mask_r,
			.mask_g = mask_g,
			.mask_b = mask_b,
			.mask_a = mask_a,
		};
		return Canvas(size, format, static_cast<uint32_t *>(pixels));
	}

	void Texture::unlock() noexcept
	{
		auto t = static_cast<SDL_Texture *>(texture);
		SDL_UnlockTexture(t);
	}

	void Texture::destroy() noexcept
	{
		SDL_DestroyTexture(static_cast<SDL_Texture *>(texture));
		texture = nullptr;
	}

	cm::Sizei Texture::size() const noexcept
	{
		cm::Sizei size;

		if (SDL_QueryTexture(static_cast<SDL_Texture *>(texture), nullptr, nullptr, &size.w, &size.h) != 0)
		{
			LOG_WARN("Could not get texture size: {}", SDL_GetError());
			return {};
		}

		return size;
	}

	void *Texture::impl() const noexcept
	{
		return texture;
	}
}
