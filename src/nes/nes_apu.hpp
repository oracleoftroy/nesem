#pragma once

#include <array>

#include "nes_types.hpp"

namespace nesem
{
	class Nes;

	// represents data for one APU channel. Not all data is used by all channels, but the used bits
	// are all in the same places for each channel type except for DMC, which is handled separately
	struct Channel
	{
		U32 data = 0;

		void set(U32 offset, U8 value) noexcept;

		U8 duty() noexcept;
		bool halt() noexcept;
		bool loop() noexcept;

		bool use_constant_volume() noexcept;
		U8 volume() noexcept;
		U8 divider() noexcept;

		bool sweep_enabled() noexcept;
		U8 sweep_period() noexcept;
		bool sweep_negate() noexcept;
		U8 sweep_shift() noexcept;

		U16 timer() noexcept;
		U8 length() noexcept;
	};

	struct Sequencer
	{
		bool start = false;

		U8 divider = 0;
		U8 decay = 0;

		// time acts as a frequency control. When this reaches 0, we move to the next position in our duty cycle and reload with the channel's timer() + 1
		U16 time = 0;

		// which bit of the duty cycle to output. APU quirk, this starts at 0 and decrements, wrapping around to bit 7, 6, ... 1, 0, 7, etc.
		U8 duty_pos = 0;

		// the duty pattern
		U8 duty = 0;

		// the length to play
		U8 length = 0;
	};

	enum class ApuStepMode
	{
		four_step,
		five_step,
	};

	class NesApu final
	{
	public:
		explicit NesApu(Nes *nes) noexcept;

		void reset() noexcept;
		void clock() noexcept;
		bool irq() noexcept;

		U8 read(U16 addr) noexcept;
		void write(U16 addr, U8 value) noexcept;

	private:
		Nes *nes;

		// channel index... in case it matters
		// TODO: delete me if unneeded
		static constexpr size_t Pulse1 = 0;
		static constexpr size_t Pulse2 = 1;
		static constexpr size_t Triangle = 2;
		static constexpr size_t Noise = 3;

		std::array<Channel, 4> channels;

		Sequencer seq_pulse_1;

		ApuStatus status = ApuStatus::none;
		ApuStepMode step_mode = ApuStepMode::four_step;
		bool frame_interrupt_enabled = false;
		bool dmc_interrupt_enabled = false;

		bool frame_interrupt_requested = false;
		bool dmc_interrupt_requested = false;

		U64 frame_counter = 0;

	private:
		void clock_quarter_frame();
		void clock_half_frame();
	};
}
