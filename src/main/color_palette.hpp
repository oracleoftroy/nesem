#pragma once

#include <filesystem>
#include <optional>
#include <span>
#include <vector>

#include <nes_types.hpp>

#include <cm/math.hpp>

namespace app
{
	constexpr nesem::U16 to_color_index(nesem::U8 color_index, util::Flags<nesem::NesColorEmphasis> emphasis) noexcept
	{
		return (color_index & 0x3F) | (emphasis.raw_value() << 5);
	}

	class ColorPalette final
	{
	public:
		static ColorPalette default_palette() noexcept;
		static std::optional<ColorPalette> from_file(const std::filesystem::path &path) noexcept;

		explicit ColorPalette() noexcept = default;
		explicit ColorPalette(std::span<cm::Color> colors) noexcept;

		cm::Color color_at_index(nesem::U16 color_index) const noexcept;

	private:
		std::vector<cm::Color> palette;
	};
}
