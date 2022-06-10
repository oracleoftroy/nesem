#include "nes_clock.hpp"

#include "nes.hpp"

#include <util/logging.hpp>

namespace nesem
{
	NesClock::NesClock(Nes *nes, ClockRate clock_rate) noexcept
		: nes(nes), clock_rate(clock_rate)
	{
		CHECK(nes != nullptr, "Nes is required");
	}

	void NesClock::tick(ClockRate::duration deltatime) noexcept
	{
		using namespace std::chrono;

		// deltatime is in seconds
		accumulator += deltatime;

		while (!force_stop && accumulator > clock_rate.frequency)
		{
			if ((tickcount % clock_rate.ppu_divisor) == 0)
				nes->ppu().clock();

			if ((tickcount % clock_rate.cpu_divisor) == 0)
				nes->cpu().clock();

			if ((tickcount % clock_rate.apu_divisor) == 0)
				nes->apu().clock();

			++tickcount;
			accumulator -= clock_rate.frequency;
		}

		force_stop = false;
	}

	ClockRate::duration NesClock::step(NesClockStep step) noexcept
	{
		using namespace std::chrono_literals;

		if (step == NesClockStep::None)
			return 0s;

		bool done = false;
		auto start_scanline = nes->ppu().current_scanline();

		// the total system time the step operation took
		ClockRate::duration deltatime = 0s;

		while (!done && !force_stop)
		{
			done = step == NesClockStep::OneClockCycle;

			if ((tickcount % clock_rate.ppu_divisor) == 0)
			{
				auto frame_complete = nes->ppu().clock();
				done = done ||
					step == NesClockStep::OnePpuCycle ||
					(frame_complete && step == NesClockStep::OneFrame) ||
					(step == NesClockStep::OnePpuScanline && nes->ppu().current_scanline() != start_scanline);
			}

			if ((tickcount % clock_rate.cpu_divisor) == 0)
			{
				auto instruction_complete = nes->cpu().clock();
				done = done || step == NesClockStep::OneCpuCycle || (instruction_complete && step == NesClockStep::OneCpuInstruction);
			}

			if ((tickcount % clock_rate.apu_divisor) == 0)
				nes->apu().clock();

			++tickcount;
			deltatime += clock_rate.frequency;
		}

		force_stop = false;
		return deltatime;
	}

	void NesClock::stop() noexcept
	{
		force_stop = true;
	}
}
