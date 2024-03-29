#pragma once

#include <cstdint>
#include <functional>
#include <string_view>
#include <version>

#include <nes_addr.hpp>

#include <util/flags.hpp>

namespace nesem
{
	using U8 = std::uint8_t;
	using U16 = std::uint16_t;
	using U32 = std::uint32_t;
	using U64 = std::uint64_t;

	enum BankSize : U32
	{
		bank_1k = 0x0400,
		bank_2k = 0x0800,
		bank_4k = 0x1000,
		bank_8k = 0x2000,
		bank_16k = 0x4000,
		bank_32k = 0x8000,
	};

	constexpr auto format_as(BankSize size) noexcept
	{
		return std::to_underlying(size);
	}

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

		// invalid state
		All = 0xFF
	};

	enum class ApuStatus : U8
	{
		none = 0x00,
		pulse_1 = 0x01,
		pulse_2 = 0x02,
		triangle = 0x04,
		noise = 0x08,
		dmc = 0x10,
	};

	enum class NesColorEmphasis : U8
	{
		none = 0x0,
		red = 0x1,
		green = 0x2,
		blue = 0x4,
	};

	// Indicate whether this read or write is ready to be handled by the issuing device
	// the 6502 always issues a read or write every cycle. Roughly equal to the CPU M2 pin
	enum class NesBusOp
	{
		pending,
		ready,
	};

#if defined(__cpp_lib_move_only_function)
	template <typename Fn>
	using CallbackFn = std::move_only_function<Fn>;
#else
	template <typename Fn>
	using CallbackFn = std::function<Fn>;
#endif

	using DrawFn = CallbackFn<void(int x, int y, U8 color_index, util::Flags<NesColorEmphasis> emphasis)>;
	using FrameReadyFn = CallbackFn<void()>;
	using PollInputFn = CallbackFn<U8()>;
	using ErrorFn = CallbackFn<void(std::string_view message)>;
}
