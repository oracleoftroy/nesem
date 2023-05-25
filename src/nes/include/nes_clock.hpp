#pragma once

#include <chrono>

#include <nes_types.hpp>

namespace nesem
{
	class Nes;

	// see: https://www.nesdev.org/wiki/Cycle_reference_chart
	struct ClockRate
	{
		using duration = std::chrono::nanoseconds;
		duration frequency{0};
		U64 cpu_divisor = 0;
		U64 ppu_divisor = 0;
		U64 apu_divisor = 0;
	};

	constexpr ClockRate ntsc() noexcept
	{
		// NTSC timings happen to be evenly divisible by 4 to an even 1 CPU/APU tick per 3 PPU tick
		// contrast that with the 16 by 5 ratio used for PAL
		constexpr U64 common_divisor = 4;

		using clockrate = std::ratio<11, 236250000 / common_divisor>;
		using duration = std::chrono::duration<U64, clockrate>;
		constexpr U64 ppu_divisor = 4 / common_divisor;
		constexpr U64 cpu_divisor = 12 / common_divisor;

		// APU is clocked half as often as the cpu
		constexpr U64 apu_divisor = (cpu_divisor * 2) / common_divisor;

		return {
			.frequency = duration_cast<ClockRate::duration>(duration(1)),
			.cpu_divisor = cpu_divisor,
			.ppu_divisor = ppu_divisor,
			.apu_divisor = apu_divisor,
		};
	}

	constexpr ClockRate pal() noexcept
	{
		using clockrate = std::ratio<10, 266017125>;
		using duration = std::chrono::duration<U64, clockrate>;
		constexpr U64 ppu_divisor = 5;
		constexpr U64 cpu_divisor = 16;

		// APU is clocked half as often as the cpu
		constexpr U64 apu_divisor = (cpu_divisor * 2);

		return {
			.frequency = duration_cast<ClockRate::duration>(duration(1)),
			.cpu_divisor = cpu_divisor,
			.ppu_divisor = ppu_divisor,
			.apu_divisor = apu_divisor,
		};
	}

	constexpr ClockRate dendy() noexcept
	{
		using clockrate = std::ratio<10, 266017125>;
		using duration = std::chrono::duration<U64, clockrate>;
		constexpr U64 ppu_divisor = 5;
		constexpr U64 cpu_divisor = 15;

		// APU is clocked half as often as the cpu
		constexpr U64 apu_divisor = (cpu_divisor * 2);

		return {
			.frequency = duration_cast<ClockRate::duration>(duration(1)),
			.cpu_divisor = cpu_divisor,
			.ppu_divisor = ppu_divisor,
			.apu_divisor = apu_divisor,
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
		ClockRate::duration accumulator{0};
	};
}
