#pragma once

#include <chrono>

#include "nes_types.hpp"

namespace nesem
{
	class Nes;

	// see: https://www.nesdev.org/wiki/Cycle_reference_chart
	struct ClockRate
	{
		using duration = std::chrono::duration<double, std::nano>;
		duration frequency{0.0};
		U64 cpu_divisor = 0;
		U64 ppu_divisor = 0;
		U64 apu_divisor = 0;
	};

	constexpr ClockRate ntsc() noexcept
	{
		// NTSC timings happen to be evenly divisiable by 4 to an even 1 CPU/APU tick per 3 PPU tick
		// contrast that with the 16 by 5 ratio used for PAL
		constexpr U64 common_divisor = 4;

		using clockrate_NTSC = std::ratio<11, 236250000 / common_divisor>;
		using duration_NTSC = std::chrono::duration<double, clockrate_NTSC>;
		constexpr U64 ppu_divisor_NTSC = 4 / common_divisor;
		constexpr U64 cpu_divisor_NTSC = 12 / common_divisor;

		// APU is clocked half as often as the cpu
		constexpr U64 apu_divisor_NTSC = 24 / common_divisor;

		return {
			.frequency = duration_cast<ClockRate::duration>(duration_NTSC(1)),
			.cpu_divisor = cpu_divisor_NTSC,
			.ppu_divisor = ppu_divisor_NTSC,
			.apu_divisor = apu_divisor_NTSC,
		};
	}

	constexpr ClockRate pal() noexcept
	{
		using clockrate_PAL = std::ratio<10, 266017125>;
		using duration_PAL = std::chrono::duration<double, clockrate_PAL>;
		constexpr U64 ppu_divisor_PAL = 5;
		constexpr U64 cpu_divisor_PAL = 16;

		// APU is clocked half as often as the cpu
		constexpr U64 apu_divisor_PAL = 32;

		return {
			.frequency = duration_cast<ClockRate::duration>(duration_PAL(1)),
			.cpu_divisor = cpu_divisor_PAL,
			.ppu_divisor = ppu_divisor_PAL,
			.apu_divisor = apu_divisor_PAL,
		};
	}

	enum class NesClockStep
	{
		None,
		OneClockCycle,
		OnePpuCycle,
		OnePpuScanline,
		OneCpuCycle,
		OneCpuInstruction,
		OneFrame,
	};

	class NesClock final
	{
	public:
		explicit NesClock(Nes *nes, ClockRate clock_rate = ntsc()) noexcept;

		void tick(ClockRate::duration deltatime) noexcept;
		ClockRate::duration step(NesClockStep step) noexcept;

		// force stop the current tick/step. Used to preserve system state on an error
		void stop() noexcept;

	private:
		Nes *nes;
		ClockRate clock_rate;

		bool force_stop = false;
		U64 tickcount = 0;
		ClockRate::duration accumulator{0.0};
	};
}
