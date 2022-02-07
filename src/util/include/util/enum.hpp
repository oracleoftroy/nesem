#pragma once

#include <concepts>
#include <type_traits>

namespace util
{
	template <typename T>
	constexpr bool is_flags_enum(T) noexcept
	{
		return false;
	}

#define MAKE_FLAGS_ENUM(EnumType)                                                           \
	constexpr bool is_flags_enum(EnumType) noexcept                                         \
	{                                                                                       \
		static_assert(std::is_enum_v<EnumType>, "is_flags_enum defined for non-enum type"); \
		return true;                                                                        \
	}

	template <typename T>
	concept flags_enum = std::is_enum_v<T> && is_flags_enum(T{});

	constexpr auto to_underlying(flags_enum auto value) noexcept
	{
		return static_cast<std::underlying_type_t<decltype(value)>>(value);
	}
}

template <util::flags_enum E>
constexpr E &operator^=(E &lhs, E rhs) noexcept
{
	lhs = static_cast<E>(
		util::to_underlying(lhs) ^=
		util::to_underlying(rhs));

	return lhs;
}

template <util::flags_enum E>
constexpr E operator^(E lhs, E rhs) noexcept
{
	lhs ^= rhs;
	return lhs;
}

template <util::flags_enum E>
constexpr E operator~(E rhs) noexcept
{
	return static_cast<E>(
		~util::to_underlying(rhs));
}

template <util::flags_enum E>
constexpr E &operator|=(E &lhs, E rhs) noexcept
{
	lhs = static_cast<E>(
		util::to_underlying(lhs) |
		util::to_underlying(rhs));

	return lhs;
}

template <util::flags_enum E>
constexpr E operator|(E lhs, E rhs) noexcept
{
	lhs |= rhs;
	return lhs;
}

template <util::flags_enum E>
constexpr E &operator&=(E &lhs, E rhs) noexcept
{
	lhs = static_cast<E>(
		util::to_underlying(lhs) &
		util::to_underlying(rhs));

	return lhs;
}

template <util::flags_enum E>
constexpr E operator&(E lhs, E rhs) noexcept
{
	lhs &= rhs;
	return lhs;
}

template <util::flags_enum E>
constexpr bool operator!(E lhs) noexcept
{
	return !util::to_underlying(lhs);
}

template <util::flags_enum E>
constexpr bool all(E value, E flags) noexcept
{
	return value & flags == flags;
}

template <util::flags_enum E>
constexpr bool any(E value, E flags) noexcept
{
	return util::to_underlying(value) & util::to_underlying(flags);
}

template <util::flags_enum E>
constexpr bool none(E value, E flags) noexcept
{
	return !(util::to_underlying(value) & util::to_underlying(flags));
}
