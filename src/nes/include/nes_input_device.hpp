#pragma once

#include <memory>
#include <utility>

#include <nes_types.hpp>

namespace nesem
{
	class NesInputDevice
	{
	public:
		NesInputDevice(PollInputFn poll_fn)
			: poll_fn(std::move(poll_fn))
		{
		}

		virtual ~NesInputDevice() = default;

		void poll() noexcept
		{
			data = poll_fn();
		}

		U8 read() noexcept
		{
			return on_read();
		}

		NesInputDevice(const NesInputDevice &other) noexcept = delete;
		NesInputDevice(NesInputDevice &&other) noexcept = delete;
		NesInputDevice &operator=(const NesInputDevice &other) noexcept = delete;
		NesInputDevice &operator=(NesInputDevice &&other) noexcept = delete;

	protected:
		U8 data = 0;

	private:
		virtual U8 on_read() noexcept
		{
			poll();
			return data;
		}

		PollInputFn poll_fn;
	};

	class NesController final : public NesInputDevice
	{
	public:
		NesController(PollInputFn poll_fn)
			: NesInputDevice(std::move(poll_fn))
		{
		}

	private:
		U8 on_read() noexcept override
		{
			// Open bus behavior will write the high 3 bits of the address into bits 5-7. Some games (e.g. paperboy) rely on this to detect buttons
			// see: https://wiki.nesdev.org/w/index.php?title=Open_bus_behavior
			U8 result = 0x40 | (data & 1);
			data >>= 1;
			data |= 0b10000000;

			return result;
		}
	};

	inline std::unique_ptr<NesInputDevice> make_null_input()
	{
		return std::make_unique<NesInputDevice>([] { return U8(0); });
	}
}
