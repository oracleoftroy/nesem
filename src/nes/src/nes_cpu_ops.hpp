#pragma once

#include <utility>

#include <nes_types.hpp>

namespace nesem::op
{
	struct ResultALU
	{
		U8 ans;
		util::Flags<ProcessorStatus> flags;
	};

	// Add with carry
	// essentially a + b + Carry
	constexpr ResultALU ADC(U8 a, U8 b, util::Flags<ProcessorStatus> flags) noexcept
	{
		using enum ProcessorStatus;

		// carry bit is already 1, so just cast to number
		auto carry = U8(flags.is_set(C));
		auto ans = a + b + carry;

		// set C if 9th bit set
		flags.set(ans & 0x0100, C);

		auto ans_sign = ans & 0b1000'0000;
		auto sign_same = (a & 0b1000'0000) == (b & 0b1000'0000);

		// set V if 8th bit changed due to overflow. This can only happen when adding two positive or two negative numbers
		// 5 + -7 -> not overflow
		// -5 + 7 -> not overflow
		// 127 + 1 -> overflow
		// -128 + -1 -> overflow
		flags.set(sign_same && ans_sign != (a & 0b1000'0000), V);

		// set N if 8th bit set
		flags.set(ans_sign, N);

		// set Z if ans == 0
		flags.set((ans & 0xFF) == 0, Z);

		return {
			.ans = U8(ans & 0xFF),
			.flags = flags,
		};
	}

	// Subtract Memory from Accumulator with Borrow
	constexpr ResultALU SBC(U8 a, U8 b, util::Flags<ProcessorStatus> flags) noexcept
	{
		// essentially, is just an add with the bits of b flipped. The programmer
		// is expected to set the carry flag to handle two's compliment properly
		// e.g. 5 - 3 becomes
		//      5 + 252 (where 252 is -4) + Carry
		return ADC(a, ~b, flags);
	}

	// AND Memory with Accumulator
	constexpr ResultALU AND(U8 a, U8 b, util::Flags<ProcessorStatus> flags) noexcept
	{
		using enum ProcessorStatus;

		U8 ans = a & b;

		flags.set(ans == 0, Z);
		flags.set(ans & 0b1000'0000, N);

		return {
			.ans = ans,
			.flags = flags,
		};
	}

	// OR Memory with Accumulator
	constexpr ResultALU ORA(U8 a, U8 b, util::Flags<ProcessorStatus> flags) noexcept
	{
		using enum ProcessorStatus;

		U8 ans = a | b;

		flags.set(ans == 0, Z);
		flags.set(ans & 0b1000'0000, N);

		return {
			.ans = ans,
			.flags = flags,
		};
	}

	// Exclusive OR Memory with Accumulator
	constexpr ResultALU EOR(U8 a, U8 b, util::Flags<ProcessorStatus> flags) noexcept
	{
		using enum ProcessorStatus;

		auto ans = U8(a ^ b);

		flags.set(ans == 0, Z);
		flags.set(ans & 0b1000'0000, N);

		return {
			.ans = ans,
			.flags = flags,
		};
	}

	// Compare Memory and Accumulator
	constexpr util::Flags<ProcessorStatus> CMP(U8 a, U8 b, util::Flags<ProcessorStatus> flags) noexcept
	{
		using enum ProcessorStatus;

		flags.set(a == b, Z);

		auto ans = U8(a - b);
		flags.set(ans & 0b1000'0000, N);
		flags.set(a >= b, C);

		return flags;
	}

	// Test Bits in Memory with Accumulator
	constexpr util::Flags<ProcessorStatus> BIT(U8 a, U8 b, util::Flags<ProcessorStatus> flags) noexcept
	{
		using enum ProcessorStatus;

		auto ans = a & b;

		// The bit instruction affects the N flag with N being set to the value of bit 7 of the memory being tested,..
		flags.set(b & 0b1000'0000, N);

		// the V flag with V being set equal to bit 6 of the memory being tested...
		flags.set(b & 0b0100'0000, V);

		// and Z being set by the result of the AND operation between the accumulator and the memory if the result is Zero, Z is reset otherwise.
		flags.set(ans == 0, Z);

		return flags;
	}

	// Arithmetic Shift Left
	constexpr ResultALU ASL(U8 value, util::Flags<ProcessorStatus> flags) noexcept
	{
		using enum ProcessorStatus;

		// if we shift off bit, store it in carry
		flags.set(value & 0b1000'0000, C);

		value <<= 1;

		flags.set(value == 0, Z);
		flags.set(value & 0b1000'0000, N);

		return {
			.ans = value,
			.flags = flags,
		};
	}

	// Logical Shift Right
	constexpr ResultALU LSR(U8 value, util::Flags<ProcessorStatus> flags) noexcept
	{
		using enum ProcessorStatus;

		// if we shift off bit, store it in carry
		flags.set(value & 1, C);

		value >>= 1;

		flags.set(value == 0, Z);
		flags.clear(N);

		return {
			.ans = value,
			.flags = flags,
		};
	}

	// Rotate Left
	constexpr ResultALU ROL(U8 value, util::Flags<ProcessorStatus> flags) noexcept
	{
		using enum ProcessorStatus;

		// Like ROR, this is a 9-bit rotate such that all bits of value are shifted left, what was in bit 7 is stored in Carry, and Carry is moved to bit 0.
		auto ans = (value << 1) | U8(flags.is_set(C));

		flags.set(ans & 0xFF00, C);
		flags.set((ans & 0xFF) == 0, Z);
		flags.set(ans & 0b1000'0000, N);

		return {
			.ans = U8(ans),
			.flags = flags,
		};
	}

	// Rotate Right
	constexpr ResultALU ROR(U8 value, util::Flags<ProcessorStatus> flags) noexcept
	{
		using enum ProcessorStatus;

		auto tmp = value & 1;

		// Like ROL, this is a 9-bit rotate such that all bits of value are shifted right, what was in bit 0 is stored in Carry, and Carry is moved to bit 7.
		auto ans = (value >> 1) | (U8(flags.is_set(C)) << 7);

		flags.set(tmp, C);
		flags.set(ans == 0, Z);
		flags.set(ans & 0b1000'0000, N);

		return {
			.ans = U8(ans),
			.flags = flags,
		};
	}

	namespace detail
	{
		constexpr void ADC_tests()
		{
			using enum ProcessorStatus;

			constexpr auto to16 = [](auto hi, auto lo) {
				return ((hi & 0xFF) << 8) | (lo & 0xFF);
			};

			// Example 2.1
			{
				constexpr auto r1 = ADC(13, 211, C);
				static_assert(r1.ans == 225);
				static_assert(r1.flags.is_set(N));

				constexpr auto r2 = ADC(13, 211, All);
				static_assert(r2.ans == 225);
				static_assert(r2.flags.is_set(I, D, B, E, N));
			}

			// Example 2.2
			{
				constexpr auto r1 = ADC(254, 6, C);
				static_assert(r1.ans == 5);
				static_assert(r1.flags.is_set(C));

				constexpr auto r2 = ADC(254, 6, All);
				static_assert(r2.ans == 5);
				static_assert(r2.flags.is_set(I, D, B, E, C));
			}

			// Example 2.4
			{
				// 16-bit add - 258 + 4112
				constexpr int ex4a = 258;
				constexpr int ex4b = 4112;

				// add the low bits
				constexpr auto r1 = ADC(U8(ex4a & 0xFF), U8(ex4b & 0xFF), None);
				static_assert(r1.ans == 18);
				static_assert(r1.flags == None);

				// carry over flags from last op
				constexpr auto r2 = ADC(U8(ex4a >> 8), U8(ex4b >> 8), r1.flags);
				static_assert(r2.ans == 17);
				static_assert(r2.flags == None);

				static_assert(to16(r2.ans, r1.ans) == 4370);
			}

			// Example 2.5
			{
				// 16-bit add - 258 + 4112
				constexpr int ex5a = 384;
				constexpr int ex5b = 128;

				// add the low bits
				constexpr auto r1 = ADC(U8(ex5a & 0xFF), U8(ex5b & 0xFF), None);
				static_assert(r1.ans == 0);
				static_assert(r1.flags.is_set(C, V, Z));

				// carry over flags from last op
				constexpr auto r2 = ADC(U8(ex5a >> 8), U8(ex5b >> 8), r1.flags);
				static_assert(r2.ans == 2);
				static_assert(r2.flags == None);

				static_assert(to16(r2.ans, r1.ans) == 512);
			}

			// Example 2.6
			{
				constexpr auto r = ADC(5, 7, None);
				static_assert(r.ans == 12);
				static_assert(r.flags == None);
			}

			// Example 2.7
			{
				constexpr auto r = ADC(127, 2, None);
				static_assert(r.ans == U8(-127));
				static_assert(r.flags.is_set(V, N));
			}

			// Example 2.8
			{
				constexpr auto r = ADC(5, U8(-3), None);
				static_assert(r.ans == 2);
				static_assert(r.flags == C);
			}

			// Example 2.9
			{
				constexpr auto r = ADC(5, U8(-7), None);
				static_assert(r.ans == U8(-2));
				static_assert(r.flags == (N));
			}

			// Example 2.10
			{
				constexpr auto r = ADC(U8(-5), U8(-7), None);
				static_assert(r.ans == U8(-12));
				static_assert(r.flags.is_set(C, N));
			}

			// Example 2.11
			{
				constexpr auto r = ADC(U8(-66), U8(-65), None);
				static_assert(r.ans == 125);
				static_assert(r.flags.is_set(C, V));
			}
		}

		constexpr void SBC_tests()
		{
			using enum ProcessorStatus;

			constexpr auto to16 = [](auto hi, auto lo) {
				return ((hi & 0xFF) << 8) | (lo & 0xFF);
			};

			// Example 2.13
			{
				constexpr auto r = SBC(U8(5), U8(3), C);
				static_assert(r.ans == 2);
				static_assert(r.flags == (C));
			}

			// Example 2.14
			{
				constexpr auto r = SBC(U8(5), U8(6), C);
				static_assert(r.ans == U8(-1));
				static_assert(r.flags == N);
			}

			// Example 2.16
			{
				// 16-bit subtract - 512 + 255
				constexpr int ex16a = 512;
				constexpr int ex16b = 255;

				// add the low bits
				constexpr auto r1 = SBC(U8(ex16a & 0xFF), U8(ex16b & 0xFF), C);
				static_assert(r1.ans == 1);
				static_assert(r1.flags == None);

				// carry over flags from last op
				constexpr auto r2 = SBC(U8(ex16a >> 8), U8(ex16b >> 8), r1.flags);
				static_assert(r2.ans == 1);
				static_assert(r2.flags == C);

				static_assert(to16(r2.ans, r1.ans) == 257);
			}

			// Example 2.17
			{
				// 16-bit subtract - 255 + 512
				constexpr int ex17a = 255;
				constexpr int ex17b = 512;

				// add the low bits
				constexpr auto r1 = SBC(U8(ex17a & 0xFF), U8(ex17b & 0xFF), C);
				static_assert(r1.ans == U8(-1));
				static_assert(r1.flags.is_set(N, C));

				// carry over flags from last op
				constexpr auto r2 = SBC(U8(ex17a >> 8), U8(ex17b >> 8), r1.flags);
				static_assert(r2.ans == U8(-2));
				static_assert(r2.flags == N);

				static_assert(to16(r2.ans, r1.ans) == (U16(-257 & 0xffff)));
			}
		}
	}
}
