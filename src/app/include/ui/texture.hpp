#pragma once

#include <ui/canvas.hpp>

namespace ui
{
	class Texture final
	{
	private:
		class TextureLock;
		struct LockData;

	public:
		Texture() noexcept = default;
		explicit Texture(void *texture) noexcept;
		~Texture();

		Texture(Texture &&texture) noexcept;
		Texture &operator=(Texture &&texture) noexcept;

		Texture(const Texture &texture) noexcept = delete;
		Texture &operator=(const Texture &texture) noexcept = delete;

		void enable_blending(bool enable) noexcept;

		LockData lock() noexcept;

		Canvas unsafe_lock() noexcept;
		void unsafe_unlock() noexcept;

		void destroy() noexcept;

		cm::Sizei size() const noexcept;

		void *impl() const noexcept;

	private:
		void *texture = nullptr;

		class TextureLock final
		{
		public:
			explicit TextureLock(Texture *texture) noexcept;
			~TextureLock();

			TextureLock(TextureLock &&other) noexcept;
			TextureLock &operator=(TextureLock &&other) noexcept;

			TextureLock(const TextureLock &other) noexcept = delete;
			TextureLock &operator=(const TextureLock &other) noexcept = delete;

			void unlock() noexcept;

		private:
			Texture *texture = nullptr;
		};

		struct LockData
		{
			Canvas canvas;
			TextureLock lock;
		};
	};
}
