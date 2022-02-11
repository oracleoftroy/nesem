#pragma once

#include <cstdint>

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
}
