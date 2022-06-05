#pragma once

namespace ui
{
	class Canvas;

	class Texture final
	{
	public:
		Texture() noexcept = default;
		explicit Texture(void *texture) noexcept;
		~Texture();

		Texture(Texture &&texture) noexcept;
		Texture &operator=(Texture &&texture) noexcept;

		Texture(const Texture &texture) noexcept = delete;
		Texture &operator=(const Texture &texture) noexcept = delete;

		void enable_blending(bool enable) noexcept;

		Canvas lock() noexcept;
		void unlock() noexcept;

		void destroy() noexcept;

		cm::Sizei size() const noexcept;

		void *impl() const noexcept;

	private:
		void *texture = nullptr;
	};
}
