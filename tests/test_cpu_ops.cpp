#include <array>
#include <string_view>

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include "../src/nes/src/nes_cpu_ops.hpp"

constexpr nesem::U16 to16(nesem::U8 hi, nesem::U8 lo) noexcept
{
	return nesem::U16(((hi & 0xFF) << 8) | (lo & 0xFF));
}

namespace Catch
{
	template <>
	struct StringMaker<util::Flags<nesem::ProcessorStatus>>
	{
		static std::string convert(util::Flags<nesem::ProcessorStatus> value)
		{
			using enum nesem::ProcessorStatus;

			auto opts = std::array{"C", "Z", "I", "D", "B", "E", "V", "N"};

			if (value == None)
				return "None";

			std::vector<std::string_view> flags;

			for (size_t i = 0; i < size(opts); ++i)
			{
				auto bit = (1 << i);

				if (value.is_set(nesem::ProcessorStatus(bit)))
					flags.emplace_back(opts[i]);
			}

			return fmt::format("{}", fmt::join(rbegin(flags), rend(flags), "|"));
		}
	};
}

TEST_CASE("ADC", "[nes_cpu]")
{
	using namespace nesem;
	using namespace nesem::op;
	using enum ProcessorStatus;

	// Examples comes from MCS6500 MICROCOMPUTER FAMILY PROGRAMMING MANUAL
	// http://users.telenet.be/kim1-6502/6502/proman.html#3

	SECTION("Example 2.1")
	{
		auto r1 = ADC(13, 211, C);
		CHECK(r1.ans == 225);
		CHECK(r1.flags.is_set(N));

		auto r2 = ADC(13, 211, All);
		CHECK(r2.ans == 225);
		CHECK(r2.flags.is_set(I, D, B, E, N));
	}

	SECTION("Example 2.2")
	{
		auto r1 = ADC(254, 6, C);
		CHECK(r1.ans == 5);
		CHECK(r1.flags == (C));

		auto r2 = ADC(254, 6, All);
		CHECK(r2.ans == 5);
		CHECK(r2.flags.is_set(I, D, B, E, C));
	}

	SECTION("Example 2.4")
	{
		// 16-bit add - 258 + 4112
		int ex4a = 258;
		int ex4b = 4112;

		// add the low bits
		auto r1 = ADC(U8(ex4a & 0xFF), U8(ex4b & 0xFF), None);
		CHECK(r1.ans == 18);
		CHECK(r1.flags.is_clear());

		// carry over flags from last op
		auto r2 = ADC(U8(ex4a >> 8), U8(ex4b >> 8), r1.flags);
		CHECK(r2.ans == 17);
		CHECK(r2.flags.is_clear());

		CHECK(to16(r2.ans, r1.ans) == 4370);
	}

	SECTION("Example 2.5")
	{
		// 16-bit add - 258 + 4112
		int ex5a = 384;
		int ex5b = 128;

		// add the low bits
		auto r1 = ADC(U8(ex5a & 0xFF), U8(ex5b & 0xFF), None);
		CHECK(r1.ans == 0);
		CHECK(r1.flags.is_set(C, V, Z));

		// carry over flags from last op
		auto r2 = ADC(U8(ex5a >> 8), U8(ex5b >> 8), r1.flags);
		CHECK(r2.ans == 2);
		CHECK(r2.flags.is_clear());

		CHECK(to16(r2.ans, r1.ans) == 512);
	}

	SECTION("Example 2.6")
	{
		auto r = ADC(5, 7, None);
		CHECK(r.ans == 12);
		CHECK(r.flags.is_clear());
	}

	SECTION("Example 2.7")
	{
		auto r = ADC(127, 2, None);
		CHECK(r.ans == U8(-127));
		CHECK(r.flags.is_set(V, N));
	}

	SECTION("Example 2.8")
	{
		auto r = ADC(5, U8(-3), None);
		CHECK(r.ans == 2);
		CHECK(r.flags == C);
	}

	SECTION("Example 2.9")
	{
		auto r = ADC(5, U8(-7), None);
		CHECK(r.ans == U8(-2));
		CHECK(r.flags == (N));
	}

	SECTION("Example 2.10")
	{
		auto r = ADC(U8(-5), U8(-7), None);
		CHECK(r.ans == U8(-12));
		CHECK(r.flags.is_set(C, N));
	}

	SECTION("Example 2.11")
	{
		auto r = ADC(U8(-66), U8(-65), None);
		CHECK(r.ans == 125);
		CHECK(r.flags.is_set(C, V));
	}
}

TEST_CASE("SBC", "[nes_cpu]")
{
	using namespace nesem;
	using namespace nesem::op;
	using enum ProcessorStatus;

	// Examples comes from MCS6500 MICROCOMPUTER FAMILY PROGRAMMING MANUAL
	// http://users.telenet.be/kim1-6502/6502/proman.html#3

	SECTION("Example 2.13")
	{
		auto r = SBC(U8(5), U8(3), C);
		CHECK(r.ans == 2);
		CHECK(r.flags == (C));
	}

	SECTION("Example 2.14")
	{
		auto r = SBC(U8(5), U8(6), C);
		CHECK(r.ans == U8(-1));
		CHECK(r.flags == N);
	}

	SECTION("Example 2.16")
	{
		// 16-bit subtract - 512 + 255
		int ex16a = 512;
		int ex16b = 255;

		// add the low bits
		auto r1 = SBC(U8(ex16a & 0xFF), U8(ex16b & 0xFF), C);
		CHECK(r1.ans == 1);
		CHECK(r1.flags.is_clear());

		// carry over flags from last op
		auto r2 = SBC(U8(ex16a >> 8), U8(ex16b >> 8), r1.flags);
		CHECK(r2.ans == 1);
		CHECK(r2.flags == C);

		CHECK(to16(r2.ans, r1.ans) == 257);
	}

	SECTION("Example 2.17")
	{
		// 16-bit subtract - 255 + 512
		int ex17a = 255;
		int ex17b = 512;

		// add the low bits
		auto r1 = SBC(U8(ex17a & 0xFF), U8(ex17b & 0xFF), C);
		CHECK(r1.ans == U8(-1));
		CHECK(r1.flags.is_set(N, C));

		// carry over flags from last op
		auto r2 = SBC(U8(ex17a >> 8), U8(ex17b >> 8), r1.flags);
		CHECK(r2.ans == U8(-2));
		CHECK(r2.flags == N);

		CHECK(to16(r2.ans, r1.ans) == (U16(-257 & 0xffff)));
	}
}

TEST_CASE("AND", "[nes_cpu]")
{
	using namespace nesem;
	using namespace nesem::op;
	using enum ProcessorStatus;

	SECTION("same masks high")
	{
		auto res = AND(0xF0, 0xF0, None);
		CHECK(res.ans == 0xF0);
		CHECK(res.flags == N);
	}

	SECTION("same masks high, other flags untouched")
	{
		auto res = AND(0xF0, 0xF0, All);
		CHECK(res.ans == 0xF0);
		// zero will be unset, N set, and the rest untouched
		CHECK(res.flags.is_clear(Z));
		CHECK(res.flags.is_set(C, I, D, B, E, V, N));
	}

	SECTION("same masks low")
	{
		auto res = AND(0x0F, 0x0F, None);
		CHECK(res.ans == 0x0F);
		CHECK(res.flags.is_clear());
	}

	SECTION("same masks low, other flags untouched")
	{
		auto res = AND(0x0F, 0x0F, All);
		CHECK(res.ans == 0x0F);
		// Zero and Negative unset, the rest untouched
		CHECK(res.flags.is_clear(N, Z));
		CHECK(res.flags.is_set(C, I, D, B, E, V));
	}

	SECTION("different masks")
	{
		auto res = AND(0xF0, 0x0F, None);
		CHECK(res.ans == 0);
		CHECK(res.flags == Z);
	}

	SECTION("different masks, other flags untouched")
	{
		auto res = AND(0xF0, 0x0F, All);
		CHECK(res.ans == 0);
		CHECK(res.flags.is_clear(N));
		CHECK(res.flags.is_set(C, I, D, B, E, V, Z));
	}
}

TEST_CASE("ORA", "[nes_cpu]")
{
	using namespace nesem;
	using namespace nesem::op;
	using enum ProcessorStatus;

	SECTION("Or all bits")
	{
		auto res = ORA(0xF0, 0x0F, None);
		CHECK(res.ans == 0xFF);
		CHECK(res.flags == N);
	}

	SECTION("Or some bits")
	{
		auto res = ORA(0x70, 0x0F, None);
		CHECK(res.ans == 0x7F);
		CHECK(res.flags.is_clear());
	}

	SECTION("Or no bits")
	{
		auto res = ORA(0, 0, None);
		CHECK(res.ans == 0);
		CHECK(res.flags == Z);
	}

	SECTION("Or all bits, other flags untouched")
	{
		auto res = ORA(0xF0, 0x0F, All);
		CHECK(res.ans == 0xFF);
		CHECK(res.flags.is_clear(Z));
		CHECK(res.flags.is_set(C, I, D, B, E, V, N));
	}

	SECTION("Or some bits, other flags untouched")
	{
		auto res = ORA(0x70, 0x0F, All);
		CHECK(res.ans == 0x7F);
		CHECK(res.flags.is_clear(Z, N));
		CHECK(res.flags.is_set(C, I, D, B, E, V));
	}

	SECTION("Or no bits, other flags untouched")
	{
		auto res = ORA(0, 0, All);
		CHECK(res.ans == 0);
		CHECK(res.flags.is_clear(N));
		CHECK(res.flags.is_set(C, I, D, B, E, V, Z));
	}
}

TEST_CASE("EOR", "[nes_cpu]")
{
	using namespace nesem;
	using namespace nesem::op;
	using enum ProcessorStatus;

	SECTION("Xor all bits exclusive")
	{
		auto res = EOR(0xF0, 0x0F, None);
		CHECK(res.ans == 0xFF);
		CHECK(res.flags == N);
	}

	SECTION("Xor most bits exclusive")
	{
		auto res = EOR(0x70, 0x0F, None);
		CHECK(res.ans == 0x7F);
		CHECK(res.flags.is_clear());
	}

	SECTION("Xor all bits inclusive")
	{
		auto res = EOR(0xF0, 0xF0, None);
		CHECK(res.ans == 0);
		CHECK(res.flags == Z);
	}

	SECTION("Xor no bits")
	{
		auto res = EOR(0, 0, None);
		CHECK(res.ans == 0);
		CHECK(res.flags == Z);
	}

	SECTION("Xor all bits exclusive, other flags untouched")
	{
		auto res = EOR(0xF0, 0x0F, All);
		CHECK(res.ans == 0xFF);
		CHECK(res.flags.is_clear(Z));
		CHECK(res.flags.is_set(C, I, D, B, E, V, N));
	}

	SECTION("Xor most bits exclusive, other flags untouched")
	{
		auto res = EOR(0x70, 0x0F, All);
		CHECK(res.ans == 0x7F);
		CHECK(res.flags.is_clear(Z, N));
		CHECK(res.flags.is_set(C, I, D, B, E, V));
	}

	SECTION("Xor all bits inclusive or zero, other flags untouched")
	{
		auto res = EOR(0xF0, 0xF0, All);
		CHECK(res.ans == 0);
		CHECK(res.flags.is_clear(N));
		CHECK(res.flags.is_set(C, I, D, B, E, V, Z));
	}
}

TEST_CASE("CMP", "[nes_cpu]")
{
	using namespace nesem;
	using namespace nesem::op;
	using enum ProcessorStatus;

	SECTION("Compare a == b")
	{
		CHECK(CMP(1, 1, None).is_set(Z, C));
		CHECK(CMP(254, 254, None).is_set(Z, C));

		CHECK(CMP(1, 1, All).is_clear(N));
		CHECK(CMP(254, 254, All).is_clear(N));
	}

	SECTION("Compare a < b")
	{
		CHECK(CMP(1, 5, None) == N);
		CHECK(CMP(252, 254, None) == N);

		CHECK(CMP(1, 5, All).is_clear(Z, C));
		CHECK(CMP(252, 254, All).is_clear(Z, C));
	}

	SECTION("Compare a > b")
	{
		CHECK(CMP(5, 1, None) == C);
		CHECK(CMP(254, 252, None) == C);

		CHECK(CMP(5, 1, All).is_clear(Z, N));
		CHECK(CMP(254, 252, All).is_clear(Z, N));
	}

	SECTION("Compare buggu")
	{
		CHECK(CMP(0x80, 0, {N, E, I}).is_set(N, E, I, C));
	}
}

TEST_CASE("BIT", "[nes_cpu]")
{
	using namespace nesem;
	using namespace nesem::op;
	using enum ProcessorStatus;

	SECTION("Bit comparisons")
	{
		CHECK(BIT(1, 1, None) == None);
		CHECK(BIT(1, 1, All).is_clear(N, V, Z));

		CHECK(BIT(1, 2, None) == Z);
		CHECK(BIT(1, 2, All).is_clear(N, V));
	}

	SECTION("Special handling of b")
	{
		// Z flagged because no bits compared equal, but N and V are special tests
		// on the second argument and independent of the accumulator
		CHECK(BIT(0, 0x80, None).is_set(N, Z));
		CHECK(BIT(0, 0x40, None).is_set(V, Z));
		CHECK(BIT(0, 0xC0, None).is_set(N, V, Z));

		CHECK(BIT(255, 0x80, All).is_clear(Z, V));
		CHECK(BIT(255, 0x40, All).is_clear(Z, N));
		CHECK(BIT(255, 0xC0, All).is_clear(Z));
	}
}

TEST_CASE("ASL", "[nes_cpu]")
{
	using namespace nesem;
	using namespace nesem::op;
	using enum ProcessorStatus;

	SECTION("0 << 1")
	{
		auto r1 = ASL(0, None);
		CHECK(r1.ans == 0);
		CHECK(r1.flags == Z);

		auto r2 = ASL(0, All);
		CHECK(r2.ans == 0);
		CHECK(r2.flags.is_clear(N, C));
	}

	SECTION("128 << 1")
	{
		auto r1 = ASL(128, None);
		CHECK(r1.ans == 0);
		CHECK(r1.flags.is_set(Z, C));

		auto r2 = ASL(128, All);
		CHECK(r2.ans == 0);
		CHECK(r2.flags.is_clear(N));
	}

	SECTION("01010101 << 1")
	{
		auto r1 = ASL(0b01010101, None);
		CHECK(r1.ans == 0b10101010);
		CHECK(r1.flags == N);

		auto r2 = ASL(0b01010101, All);
		CHECK(r2.ans == 0b10101010);
		CHECK(r2.flags.is_clear(Z, C));
	}

	SECTION("10101010 << 1")
	{
		auto r1 = ASL(0b10101010, None);
		CHECK(r1.ans == 0b01010100);
		CHECK(r1.flags == (C));

		auto r2 = ASL(0b10101010, All);
		CHECK(r2.ans == 0b01010100);
		CHECK(r2.flags.is_clear(Z, N));
	}
}

TEST_CASE("LSR", "[nes_cpu]")
{
	using namespace nesem;
	using namespace nesem::op;
	using enum ProcessorStatus;

	SECTION("0 >> 1")
	{
		auto r1 = LSR(0, None);
		CHECK(r1.ans == 0);
		CHECK(r1.flags == Z);

		auto r2 = LSR(0, All);
		CHECK(r2.ans == 0);
		CHECK(r2.flags.is_clear(N, C));
	}

	SECTION("1 >> 1")
	{
		auto r1 = LSR(1, None);
		CHECK(r1.ans == 0);
		CHECK(r1.flags.is_set(Z, C));

		auto r2 = LSR(1, All);
		CHECK(r2.ans == 0);
		CHECK(r2.flags.is_clear(N));
	}

	SECTION("01010101 >> 1")
	{
		auto r1 = LSR(0b01010101, None);
		CHECK(r1.ans == 0b00101010);
		CHECK(r1.flags == C);

		auto r2 = LSR(0b01010101, All);
		CHECK(r2.ans == 0b00101010);
		CHECK(r2.flags.is_clear(Z, N));
	}

	SECTION("10101010 >> 1")
	{
		auto r1 = LSR(0b10101010, None);
		CHECK(r1.ans == 0b01010101);
		CHECK(r1.flags.is_clear());

		auto r2 = LSR(0b10101010, All);
		CHECK(r2.ans == 0b01010101);
		CHECK(r2.flags.is_clear(C, Z, N));
	}
}

TEST_CASE("ROL", "[nes_cpu]")
{
	using namespace nesem;
	using namespace nesem::op;
	using enum ProcessorStatus;

	SECTION("0 << 1")
	{
		auto r1 = ROL(0, None);
		CHECK(r1.ans == 0);
		CHECK(r1.flags == Z);

		auto r2 = ROL(0, All);
		CHECK(r2.ans == 1);
		CHECK(r2.flags.is_clear(N, C, Z));
	}

	SECTION("128 << 1")
	{
		auto r1 = ROL(128, None);
		CHECK(r1.ans == 0);
		CHECK(r1.flags.is_set(Z, C));

		auto r2 = ROL(128, All);
		CHECK(r2.ans == 1);
		CHECK(r2.flags.is_clear(N, Z));
	}

	SECTION("01010101 << 1")
	{
		auto r1 = ROL(0b01010101, None);
		CHECK(r1.ans == 0b10101010);
		CHECK(r1.flags == N);

		auto r2 = ROL(0b01010101, All);
		CHECK(r2.ans == 0b10101011);
		CHECK(r2.flags.is_clear(Z, C));
	}

	SECTION("10101010 << 1")
	{
		auto r1 = ROL(0b10101010, None);
		CHECK(r1.ans == 0b01010100);
		CHECK(r1.flags == (C));

		auto r2 = ROL(0b10101010, All);
		CHECK(r2.ans == 0b01010101);
		CHECK(r2.flags.is_clear(Z, N));
	}

	SECTION("bit goes round the world")
	{
		auto r = ROL(1, None);
		CHECK(r.ans == 0b00000010);
		CHECK(r.flags.is_clear());

		r = ROL(r.ans, r.flags);
		CHECK(r.ans == 0b00000100);
		CHECK(r.flags.is_clear());

		r = ROL(r.ans, r.flags);
		CHECK(r.ans == 0b00001000);
		CHECK(r.flags.is_clear());

		r = ROL(r.ans, r.flags);
		CHECK(r.ans == 0b00010000);
		CHECK(r.flags.is_clear());

		r = ROL(r.ans, r.flags);
		CHECK(r.ans == 0b00100000);
		CHECK(r.flags.is_clear());

		r = ROL(r.ans, r.flags);
		CHECK(r.ans == 0b01000000);
		CHECK(r.flags.is_clear());

		r = ROL(r.ans, r.flags);
		CHECK(r.ans == 0b10000000);
		CHECK(r.flags == N);

		r = ROL(r.ans, r.flags);
		CHECK(r.ans == 0b00000000);
		CHECK(r.flags.is_set(C, Z));

		r = ROL(r.ans, r.flags);
		CHECK(r.ans == 0b00000001);
		CHECK(r.flags.is_clear());
	}
}

TEST_CASE("ROR", "[nes_cpu]")
{
	using namespace nesem;
	using namespace nesem::op;
	using enum ProcessorStatus;

	SECTION("0 >> 1")
	{
		auto r1 = ROR(0, None);
		CHECK(r1.ans == 0);
		CHECK(r1.flags == Z);

		auto r2 = ROR(0, All);
		CHECK(r2.ans == 0b1000'0000);
		CHECK(r2.flags.is_clear(Z, C));
	}

	SECTION("1 >> 1")
	{
		auto r1 = ROR(1, None);
		CHECK(r1.ans == 0);
		CHECK(r1.flags.is_set(Z, C));

		auto r2 = ROR(1, All);
		CHECK(r2.ans == 0b1000'0000);
		CHECK(r2.flags.is_clear(Z));
	}

	SECTION("01010101 >> 1")
	{
		auto r1 = ROR(0b01010101, None);
		CHECK(r1.ans == 0b00101010);
		CHECK(r1.flags == C);

		auto r2 = ROR(0b01010101, All);
		CHECK(r2.ans == 0b10101010);
		CHECK(r2.flags.is_clear(Z));
	}

	SECTION("10101010 >> 1")
	{
		auto r1 = ROR(0b10101010, None);
		CHECK(r1.ans == 0b01010101);
		CHECK(r1.flags.is_clear());

		auto r2 = ROR(0b10101010, All);
		CHECK(r2.ans == 0b11010101);
		CHECK(r2.flags.is_clear(C, Z));
	}

	SECTION("bit goes round the world")
	{
		auto r = ROR(1, None);
		CHECK(r.ans == 0b00000000);
		CHECK(r.flags.is_set(Z, C));

		r = ROR(r.ans, r.flags);
		CHECK(r.ans == 0b10000000);
		CHECK(r.flags == N);

		r = ROR(r.ans, r.flags);
		CHECK(r.ans == 0b01000000);
		CHECK(r.flags.is_clear());

		r = ROR(r.ans, r.flags);
		CHECK(r.ans == 0b00100000);
		CHECK(r.flags.is_clear());

		r = ROR(r.ans, r.flags);
		CHECK(r.ans == 0b00010000);
		CHECK(r.flags.is_clear());

		r = ROR(r.ans, r.flags);
		CHECK(r.ans == 0b00001000);
		CHECK(r.flags.is_clear());

		r = ROR(r.ans, r.flags);
		CHECK(r.ans == 0b00000100);
		CHECK(r.flags.is_clear());

		r = ROR(r.ans, r.flags);
		CHECK(r.ans == 0b00000010);
		CHECK(r.flags.is_clear());

		r = ROR(r.ans, r.flags);
		CHECK(r.ans == 0b00000001);
		CHECK(r.flags.is_clear());
	}
}
