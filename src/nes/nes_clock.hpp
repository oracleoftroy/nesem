#pragma once

#include <chrono>

#include "nes_types.hpp"

namespace nesem
{
	class NesCpu;
	class NesPpu;
	class NesApu;

	struct ClockRate
	{
		using duration = std::chrono::duration<double, std::nano>;
		duration frequency{0.0};
		U64 cpu_divisor = 0;
		U64 ppu_divisor = 0;
	};

	constexpr ClockRate ntsc() noexcept
	{
		using clockrate_NTSC = std::ratio<11, 236250000>;
		using duration_NTSC = std::chrono::duration<double, clockrate_NTSC>;
		constexpr U64 cpu_divisor_NTSC = 12;
		constexpr U64 ppu_divisor_NTSC = 4;

		return {
			.frequency = duration_cast<ClockRate::duration>(duration_NTSC(1)),
			.cpu_divisor = cpu_divisor_NTSC,
			.ppu_divisor = ppu_divisor_NTSC,
		};
	}

	constexpr ClockRate pal() noexcept
	{
		using clockrate_PAL = std::ratio<10, 266017125>;
		using duration_PAL = std::chrono::duration<double, clockrate_PAL>;
		constexpr U64 cpu_divisor_PAL = 16;
		constexpr U64 ppu_divisor_PAL = 5;

		return {
			.frequency = duration_cast<ClockRate::duration>(duration_PAL(1)),
			.cpu_divisor = cpu_divisor_PAL,
			.ppu_divisor = ppu_divisor_PAL,
		};
	}

	enum class NesClockStep
	{
		OneClockCycle,
		OnePpuCycle,
		OneCpuCycle,
		OneCpuInstruction,
		OneFrame,
	};

	class NesClock final
	{
	public:
		explicit NesClock(NesCpu *cpu, ClockRate clock_rate = ntsc()) noexcept;

		// run the system at full speed for a timeslice
		void tick(ClockRate::duration deltatime) noexcept;

		// step the system
		void step(NesClockStep step) noexcept;

	private:
		U64 tickcount = 0;
		ClockRate::duration accumulator{0.0};
		ClockRate clock_rate;

		NesCpu *cpu;
		// NesPpu *ppu;
		// NesApu *apu;
	};

}
