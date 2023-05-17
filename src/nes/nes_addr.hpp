#pragma once

#include <cstdint>
#include <type_traits>
#include <utility>

#include <fmt/format.h>

namespace nesem
{
	enum class Addr : std::uint_fast16_t
	{
	};

	constexpr auto to_integer(Addr addr) noexcept
	{
		return std::to_underlying(addr);
	}

	constexpr bool operator==(Addr a, std::integral auto b) noexcept
	{
		return std::cmp_equal(std::to_underlying(a), b);
	}

	constexpr auto operator<=>(Addr a, std::integral auto b) noexcept
	{
		if (std::cmp_less(std::to_underlying(a), b))
			return std::strong_ordering::less;
		else if (std::cmp_greater(std::to_underlying(a), b))
			return std::strong_ordering::greater;
		else
			return std::strong_ordering::equal;
	}

	constexpr Addr &operator&=(Addr &a, Addr b) noexcept
	{
		a = Addr(std::to_underlying(a) & std::to_underlying(b));
		return a;
	}

	constexpr Addr operator&(Addr a, Addr b) noexcept
	{
		a &= b;
		return a;
	}

	constexpr Addr &operator|=(Addr &a, Addr b) noexcept
	{
		a = Addr(std::to_underlying(a) | std::to_underlying(b));
		return a;
	}

	constexpr Addr operator|(Addr a, Addr b) noexcept
	{
		a |= b;
		return a;
	}

	// constexpr Addr &operator^=(Addr &a, Addr b) noexcept
	// {
	// 	a = Addr(std::to_underlying(a) ^ std::to_underlying(b));
	// 	return a;
	// }

	// constexpr Addr operator^(Addr a, Addr b) noexcept
	// {
	// 	a ^= b;
	// 	return a;
	// }

	// constexpr Addr &operator>>=(Addr &a, Addr b) noexcept
	// {
	// 	a = Addr(std::to_underlying(a) >> std::to_underlying(b));
	// 	return a;
	// }

	// constexpr Addr operator>>(Addr a, Addr b) noexcept
	// {
	// 	a >>= b;
	// 	return a;
	// }

	// constexpr Addr &operator<<=(Addr &a, Addr b) noexcept
	// {
	// 	a = Addr(std::to_underlying(a) << std::to_underlying(b));
	// 	return a;
	// }

	// constexpr Addr operator<<(Addr a, Addr b) noexcept
	// {
	// 	a <<= b;
	// 	return a;
	// }

	constexpr Addr &operator&=(Addr &a, std::integral auto b) noexcept
	{
		a = Addr(std::to_underlying(a) & b);
		return a;
	}

	constexpr Addr operator&(Addr a, std::integral auto b) noexcept
	{
		a &= b;
		return a;
	}

	constexpr Addr operator&(std::integral auto b, Addr a) noexcept
	{
		a &= b;
		return a;
	}

	constexpr Addr &operator|=(Addr &a, std::integral auto b) noexcept
	{
		a = Addr(std::to_underlying(a) | b);
		return a;
	}

	constexpr Addr operator|(Addr a, std::integral auto b) noexcept
	{
		a |= b;
		return a;
	}

	constexpr Addr operator|(std::integral auto b, Addr a) noexcept
	{
		a |= b;
		return a;
	}

	constexpr Addr &operator^=(Addr &a, std::integral auto b) noexcept
	{
		a = Addr(std::to_underlying(a) ^ b);
		return a;
	}

	constexpr Addr operator^(Addr a, std::integral auto b) noexcept
	{
		a ^= b;
		return a;
	}

	constexpr Addr operator^(std::integral auto b, Addr a) noexcept
	{
		a ^= b;
		return a;
	}

	constexpr Addr &operator>>=(Addr &a, std::integral auto b) noexcept
	{
		a = Addr(std::to_underlying(a) >> b);
		return a;
	}

	constexpr Addr operator>>(Addr a, std::integral auto b) noexcept
	{
		a >>= b;
		return a;
	}

	constexpr Addr &operator<<=(Addr &a, std::integral auto b) noexcept
	{
		a = Addr(std::to_underlying(a) << b);
		return a;
	}

	constexpr Addr operator<<(Addr a, std::integral auto b) noexcept
	{
		a <<= b;
		return a;
	}

	constexpr Addr &operator+=(Addr &a, std::integral auto b) noexcept
	{
		a = Addr(std::to_underlying(a) + b);
		return a;
	}

	constexpr Addr operator+(Addr a, std::integral auto b) noexcept
	{
		a += b;
		return a;
	}

	constexpr Addr &operator-=(Addr &a, std::integral auto b) noexcept
	{
		a = Addr(std::to_underlying(a) - b);
		return a;
	}

	constexpr Addr operator-(Addr a, std::integral auto b) noexcept
	{
		a -= b;
		return a;
	}

	constexpr Addr operator++(Addr &a) noexcept
	{
		a = Addr(std::to_underlying(a) + 1);
		return a;
	}

	constexpr Addr operator++(Addr &a, int) noexcept
	{
		auto result = a;
		a = Addr(std::to_underlying(a) + 1);
		return result;
	}

	constexpr Addr operator--(Addr &a) noexcept
	{
		a = Addr(std::to_underlying(a) - 1);
		return a;
	}

	constexpr Addr operator--(Addr &a, int) noexcept
	{
		auto result = a;
		a = Addr(std::to_underlying(a) - 1);
		return result;
	}
}

template <>
struct fmt::formatter<nesem::Addr>
{
	constexpr auto parse(format_parse_context &ctx) -> format_parse_context::iterator
	{
		auto it = ctx.begin();
		auto end = ctx.end();

		// Check if reached the end of the range:
		if (it != end && *it != '}')
			ctx.on_error("invalid format");

		// Return an iterator past the end of the parsed range:
		return it;
	}

	auto format(nesem::Addr addr, format_context &ctx) const -> format_context::iterator
	{
		return fmt::format_to(ctx.out(), "{:04X}", std::to_underlying(addr));
	}
};
