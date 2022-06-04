#pragma once

#include <cstdint>
#include <functional>
#include <version>

#include <util/enum.hpp>

namespace nesem
{
	using U8 = std::uint_fast8_t;
	using U16 = std::uint_fast16_t;
	using U32 = std::uint_fast32_t;
	using U64 = std::uint_fast64_t;

	// processor status flags
	enum class ProcessorStatus : U8
	{
		None = 0,
		C = 0x01, // Carry
		Z = 0x02, // Zero result
		I = 0x04, // Interrupt disable
		D = 0x08, // Decimal Mode
		B = 0x10, // Break Command - Used to distinguish hardware (irq, nmi) and software (php, brk) interrupt when pushing status to the stack: hw: 0, sw: 1
		E = 0x20, // Expansion (unused) - default on, hardware may be wired to high
		V = 0x40, // Overflow
		N = 0x80, // Negative result

		Default = E | I,
		All = C | Z | I | D | B | E | V | N,
	};
	MAKE_FLAGS_ENUM(ProcessorStatus);

	enum class Buttons : U8
	{
		None = 0x00,
		A = 0x01,
		B = 0x02,
		Select = 0x04,
		Start = 0x08,
		Up = 0x10,
		Down = 0x20,
		Left = 0x40,
		Right = 0x80,
	};
	MAKE_FLAGS_ENUM(Buttons);

	enum class ApuStatus : U8
	{
		none = 0x00,
		pulse_1 = 0x01,
		pulse_2 = 0x02,
		triangle = 0x04,
		noise = 0x08,
		dmc = 0x10,
	};
	MAKE_FLAGS_ENUM(ApuStatus);

#if defined(__cpp_lib_move_only_function)
	using DrawFn = std::move_only_function<void(int x, int y, int color_index)>;
	using PollInputFn = std::move_only_function<Buttons()>;
	using ErrorFn = std::move_only_function<void(std::string_view message)>;
#else
	using DrawFn = std::function<void(int x, int y, int color_index)>;
	using PollInputFn = std::function<Buttons()>;
	using ErrorFn = std::function<void(std::string_view message)>;
#endif
}
