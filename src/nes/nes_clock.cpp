#include "nes_clock.hpp"

#include <util/logging.hpp>

#include "nes.hpp"

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

		while (accumulator > clock_rate.frequency)
		{
			if ((tickcount % clock_rate.ppu_divisor) == 0)
				// tick the ppu
				nes->ppu().clock();

			if ((tickcount % clock_rate.cpu_divisor) == 0)
				// tick the cpu
				nes->cpu().clock();

			// if (tickcount % clock_rate.apu_divisor)
			// 	// tick the apu
			// 	;

			++tickcount;
			accumulator -= clock_rate.frequency;
		}
	}

	void NesClock::step(NesClockStep step) noexcept
	{
		// TODO: support stepping by frame and CPU instruction
		bool done = false;

		while (!done)
		{
			done = step == NesClockStep::OneClockCycle;

			if (tickcount % clock_rate.ppu_divisor)
			{ // tick the ppu
				nes->ppu().clock();
				done = done || step == NesClockStep::OnePpuCycle;
			}

			if (tickcount % clock_rate.cpu_divisor)
			{
				nes->cpu().clock();
				done = done || step == NesClockStep::OneCpuCycle;
			}

			++tickcount;
		}
	}
}
