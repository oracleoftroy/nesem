#pragma once

#include <compare>
#include <concepts>
#include <type_traits>
#include <utility>

namespace util
{
	template <typename T>
	requires std::is_scoped_enum_v<T>
	class Flags final
	{
	public:
		constexpr explicit Flags() noexcept = default;

		template <std::same_as<T>... Ts>
		constexpr Flags(Ts... flags) noexcept
			: underlying_value((... | std::to_underlying(flags)))
		{
		}

		template <std::same_as<T>... Ts>
		constexpr bool is_set(Ts... flags) const noexcept
		{
			auto bits = (... | std::to_underlying(flags));
			return (underlying_value & bits) == bits;
		}

		template <std::same_as<T>... Ts>
		constexpr bool any_set(Ts... flags) const noexcept
		{
			auto bits = (... | std::to_underlying(flags));
			return (underlying_value & bits) != 0;
		}

		template <std::same_as<T>... Ts>
		constexpr bool is_clear(Ts... flags) const noexcept
		{
			auto bits = (... | std::to_underlying(flags));
			return (underlying_value & bits) == 0;
		}

		constexpr bool is_clear() const noexcept
		{
			return underlying_value == 0;
		}

		template <std::same_as<T>... Ts>
		constexpr T extract(Ts... flags) const noexcept
		{
			auto bits = (... | std::to_underlying(flags));
			return static_cast<T>(underlying_value & bits);
		}

		template <std::same_as<T>... Ts>
		constexpr void set(bool value, Ts... flags) noexcept
		{
			if (value)
				set(flags...);
			else
				clear(flags...);
		}

		template <std::same_as<T>... Ts>
		constexpr void set(Ts... flags) noexcept
		{
			underlying_value |= (... | std::to_underlying(flags));
		}

		constexpr void clear() noexcept = delete;

		constexpr void clear_all() noexcept
		{
			underlying_value = 0;
		}

		template <std::same_as<T>... Ts>
		constexpr void clear(Ts... flags) noexcept
		{
			underlying_value &= ~(... | std::to_underlying(flags));
		}

		constexpr T value() const noexcept
		{
			return static_cast<T>(underlying_value);
		}

		constexpr std::underlying_type_t<T> raw_value() const noexcept
		{
			return underlying_value;
		}

		constexpr explicit operator T() const noexcept
		{
			return value();
		}

		constexpr explicit operator std::underlying_type_t<T>() const noexcept
		{
			return raw_value();
		}

		constexpr auto operator<=>(const Flags<T> &other) const noexcept = default;

	private:
		std::underlying_type_t<T> underlying_value = 0;
	};

	template <typename T, typename... Ts>
	requires std::is_scoped_enum_v<T> && (... && std::is_same_v<T, Ts>)
	Flags(T, Ts...) -> Flags<T>;

	namespace static_tests::flags
	{
		constexpr void f() noexcept
		{
			enum class binary : unsigned char
			{
				zero = 0,
				one = 1,
				two = 2,
				four = 4,
				eight = 8,
				sixteen = 16,
				thirty_two = 32,
			};

			using enum binary;

			constexpr Flags<binary> zero_v1;
			constexpr auto zero_v2 = Flags{zero};
			constexpr auto zero_v3 = zero_v2;

			static_assert(zero_v1 == zero);
			static_assert(zero_v1 == zero_v2);
			static_assert(zero_v1 == zero_v3);

			constexpr auto one_bit_v1 = Flags(four);
			constexpr auto one_bit_v2 = one_bit_v1;
			static_assert(one_bit_v1 == four);
			static_assert(one_bit_v1 == one_bit_v2);

			constexpr auto some_bits_v1 = Flags(one, four, sixteen);
			constexpr auto some_bits_v2 = some_bits_v1;
			static_assert(some_bits_v1 != four);
			static_assert(some_bits_v1 == some_bits_v2);

			constexpr Flags<binary> some_bits(one, four, sixteen);
			static_assert(some_bits.is_set(one));
			static_assert(some_bits.is_set(four));
			static_assert(some_bits.is_set(sixteen));
			static_assert(some_bits.is_set(one, four));
			static_assert(!some_bits.is_set(two));
			static_assert(!some_bits.is_set(one, two));

			static_assert(some_bits.any_set(one));
			static_assert(some_bits.any_set(one, two));
			static_assert(!some_bits.any_set(two));
			static_assert(!some_bits.any_set(two, eight));

			static_assert(some_bits.is_clear(two));
			static_assert(some_bits.is_clear(two, eight));
			static_assert(!some_bits.is_clear(one, two));

			static_assert(zero_v1.is_clear());
			static_assert(zero_v2.is_clear());
			static_assert(!some_bits.is_clear());

			static_assert(some_bits.extract(sixteen) == sixteen);
			static_assert(some_bits.extract(one, sixteen) == binary(17));
			static_assert(some_bits.extract(one, two) == one);
			static_assert(some_bits.extract(two, eight) == zero);

			constexpr auto clear_all_test = [](auto f) {
				f.clear_all();
				return f;
			};

			static_assert(clear_all_test(some_bits) == zero);

			constexpr auto clear_test = [](auto f, auto... fs) {
				f.clear(fs...);
				return f;
			};

			static_assert(clear_test(some_bits, one, four, sixteen) == zero);
			static_assert(clear_test(some_bits, one, sixteen) == Flags{four});
			static_assert(clear_test(some_bits, one) == Flags{four, sixteen});
			static_assert(clear_test(some_bits, one, two) == Flags{four, sixteen});
			static_assert(clear_test(some_bits, two, eight) == some_bits);

			constexpr auto argument_test = [](Flags<binary> args) {
				return args;
			};
			static_assert(argument_test(two) == two);
			static_assert(argument_test({two, thirty_two}).is_set(two));
			static_assert(argument_test({two, thirty_two}).is_set(thirty_two));
		}
	}
}
