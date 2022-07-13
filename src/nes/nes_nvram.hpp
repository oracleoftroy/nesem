#pragma once

#include <filesystem>
#include <memory>
#include <span>

#include "nes_types.hpp"

namespace nesem
{
	class NesNvram final
	{
	public:
		explicit NesNvram() noexcept;
		explicit NesNvram(const std::filesystem::path &file_name, size_t size) noexcept;

		~NesNvram();

		NesNvram(NesNvram &&other) noexcept;
		NesNvram &operator=(NesNvram &&other) noexcept;
		NesNvram(const NesNvram &other) noexcept = delete;
		NesNvram &operator=(const NesNvram &other) noexcept = delete;

		explicit operator bool() const noexcept
		{
			return core != nullptr;
		}

		size_t size() const noexcept
		{
			return data.size();
		}

#if defined(__cpp_explicit_this_parameter)
		auto &&operator[](this auto &&self, size_t addr) noexcept
		{
			return self.data[addr];
		}
#else
		U8 operator[](size_t addr) const noexcept
		{
			return data[addr];
		}

		U8 &operator[](size_t addr) noexcept
		{
			return data[addr];
		}
#endif

	private:
		struct Core;
		std::unique_ptr<Core> core;

		std::span<U8> data;
	};

}
