#include "nes_apu.hpp"

#include "nes.hpp"

#include <util/logging.hpp>

namespace nesem
{
	// the 2 bit duty value indexes this loopup table
	static constexpr std::array<U8, 4> duty_patterns = {
		0b01000000,
		0b01100000,
		0b01111000,
		0b10011111,
	};

	// the 5-bit length value indexes this lookup table for the correct length value
	static constexpr std::array<U8, 32> length_table = {
		10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
		12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30};

	void Channel::set(U32 offset, U8 value) noexcept
	{
		CHECK(offset < 4, "byte offset out of range of U32");

		U32 shift = offset * 8;
		U32 mask = 0xFF << shift;

		data = (data & ~mask) | (value << shift);
	}

	U8 Channel::duty() const noexcept
	{
		return (data >> 6) & 3;
	}

	bool Channel::halt() const noexcept
	{
		return (data & 0b0010'0000) > 0;
	}

	// same as halt for pulse channels?
	// TODO: err, delete one of these? Rename? keep?
	bool Channel::loop() const noexcept
	{
		return halt();
	}

	bool Channel::use_constant_volume() const noexcept
	{
		return (data & 0b0001'0000) > 0;
	}

	U8 Channel::volume() const noexcept
	{
		return data & 0xF;
	}

	// same as volume for pulse channels...
	// TODO: err, delete one of these? Rename? keep?
	U8 Channel::divider() const noexcept
	{
		return volume();
	}

	bool Channel::sweep_enabled() const noexcept
	{
		return (data & 0b1000'0000'0000'0000) > 0;
	}

	U8 Channel::sweep_period() const noexcept
	{
		return (data >> 12) & 0x7;
	}

	bool Channel::sweep_negate() const noexcept
	{
		return (data & 0b0000'1000'0000'0000) > 0;
	}

	U8 Channel::sweep_shift() const noexcept
	{
		return (data >> 8) & 0x7;
	}

	U16 Channel::timer() const noexcept
	{
		return (data >> 16) & 0x7FF;
	}

	U8 Channel::length() const noexcept
	{
		return (data >> 27) & 0x1F;
	}

	void Sweep::clock(const Channel &channel) noexcept
	{
		--period;

		if (channel.sweep_period() < 8)
		{
		}
	}

	NesApu::NesApu(Nes *nes) noexcept
		: nes(nes)
	{
	}

	void NesApu::reset() noexcept
	{
		status = ApuStatus::none;
		frame_interrupt_requested = false;
		dmc_interrupt_requested = false;
	}

	void NesApu::clock() noexcept
	{
		switch (frame_counter)
		{
		case 0:
			if (step_mode == ApuStepMode::four_step && frame_interrupt_enabled)
				frame_interrupt_requested = true;
			break;

		case 3728:
			clock_quarter_frame();
			break;

		case 7456:
			clock_quarter_frame();
			clock_half_frame();
			break;

		case 11185:
			clock_quarter_frame();
			break;

		case 14914:
			if (step_mode == ApuStepMode::four_step)
			{
				clock_quarter_frame();
				clock_half_frame();

				if (frame_interrupt_enabled)
					frame_interrupt_requested = true;

				frame_counter = 0;
			}
			break;

		case 18640:
			// TODO: figure out what we want to do here. APU isn't fully implemented, so ignore this inconsistency for now.
			// are we susposed to finish the current step and when we start over, it will be four step? there are also supposed to be delays when setting apu values...
			// CHECK(step_mode == ApuStepMode::five_step, "Invalid step mode!");

			clock_quarter_frame();
			clock_half_frame();
			frame_counter = 0;
			break;
		}

		++frame_counter;
	}

	bool NesApu::irq() noexcept
	{
		return frame_interrupt_requested || dmc_interrupt_requested;
	}

	U8 NesApu::read(U16 addr) noexcept
	{
		if (addr == 0x4015)
		{
			U8 result =
				((channels[Pulse1].length() > 0) << 0) |
				((channels[Pulse2].length() > 0) << 1) |
				((channels[Triangle].length() > 0) << 2) |
				((channels[Noise].length() > 0) << 3) |
				// ((Dmc.length() > 0) << 4) | // TODO: bytes remaining > 0
				(frame_interrupt_requested << 6) |
				(dmc_interrupt_requested << 7);

			// reading this address always clears the frame irq
			frame_interrupt_requested = false;
			return result;
		}

		LOG_WARN_ONCE("Attempted read of audio register at addr: {:04X}", addr);
		return nes->bus().open_bus_read();
	}

	void NesApu::write(U16 addr, U8 value) noexcept
	{
		// first, check for writes to addresses the apu will handle directly, as well as invalid addresses
		switch (addr)
		{
		case 0x4010:
		case 0x4011:
		case 0x4012:
		case 0x4013:
			LOG_WARN_ONCE("DMC channel not implemented, ignoring write to {:04X} with value: {:02X}", addr, value);
			return;

		case 0x4015:
			status = static_cast<ApuStatus>(value);

			// disabling a channel immediately silences and sets length to 0
			if ((status & ApuStatus::pulse_1) == ApuStatus::none)
			{
				// TODO: silence?
				seq_pulse_1.length = 0;
			}

			// TODO: do the above for the other channels

			// TODO: DMC cleared sets bytes remaining to 0 and will be silenced when empty
			// TODO: DMC set will restart sample only if bytes remaining is 0

			dmc_interrupt_requested = false;
			return;

		case 0x4017:
			frame_interrupt_enabled = (value & 0b0100'0000) == 0;

			// disabling frame interrupts clears any active frame irq
			if (!frame_interrupt_enabled)
				frame_interrupt_requested = false;

			if (value & 0b1000'0000)
				step_mode = ApuStepMode::five_step;
			else
				step_mode = ApuStepMode::four_step;

			return;

			// unused
		case 0x4009:
		case 0x400D:
		case 0x4014:
		case 0x4016:
			LOG_WARN_ONCE("Write to unused APU addr: {:04X} with value: {:02X}", addr, value);
			return;
		}

		if (addr >= 0x4000 && addr < 0x4010)
		{
			auto index = (addr >> 2) & 3;
			auto byte = addr & 3;

			channels[index].set(byte, value);

			// Unless disabled, a write the channel's fourth register immediately reloads the counter with the value from a lookup table

			if (byte == 0)
			{
				// The duty cycle is changed (see table below), but the sequencer's current position isn't affected.
				// TODO: support pulse 2
				if (index == Pulse1)
					seq_pulse_1.duty = channels[index].duty();
			}
			else if (byte == 3)
			{
				// The sequencer is immediately restarted at the first value of the current sequence. The envelope is also restarted.
				// TODO: support other channels
				if (index == Pulse1)
				{
					// TODO: rename this? I think this flags restarting the sequencer, not whether it is in a running state....
					seq_pulse_1.start = true;
					seq_pulse_1.duty_pos = 0;
					seq_pulse_1.length = length_table[channels[index].length()];
				}
			}
			return;
		}

		LOG_WARN("Out of range write to APU addr {:04X} with value {:02X}", addr, value);
	}

	void NesApu::clock_quarter_frame()
	{
		// clock envelopes

		// When clocked by the frame counter, one of two actions occurs: if the start flag is clear, the divider is clocked,
		if (!seq_pulse_1.start)
		{
			// clock divider
			// When the divider is clocked while at 0, it is loaded with V and clocks the decay level counter.

			// Then one of two actions occurs: If the counter is non-zero, it is decremented, otherwise if the loop flag is set, the decay level counter is loaded with 15.
			if (seq_pulse_1.divider == 0)
			{
				seq_pulse_1.divider = channels[Pulse1].divider();

				if (seq_pulse_1.decay != 0)
					--seq_pulse_1.decay;
				else if (channels[Pulse1].loop())
					seq_pulse_1.decay = 15;
			}
			else
				--seq_pulse_1.divider;
		}

		// otherwise the start flag is cleared, the decay level counter is loaded with 15, and the divider's period is immediately reloaded.
		else
		{
			seq_pulse_1.start = false;
			seq_pulse_1.decay = 15;
			seq_pulse_1.divider = channels[Pulse1].divider();
		}

		// The envelope unit's volume output depends on the constant volume flag: if set, the envelope parameter directly sets the volume, otherwise the decay level is the current volume. The constant volume flag has no effect besides selecting the volume source; the decay level will still be updated when constant volume is selected.

		// Each of the three envelope units' output is fed through additional gates at the sweep unit (pulse only), waveform generator (sequencer or LFSR), and length counter.

		// clock triangle linear counter
	}

	void NesApu::clock_half_frame()
	{
		// clock length counters
		// clock sweep units
	}
}
