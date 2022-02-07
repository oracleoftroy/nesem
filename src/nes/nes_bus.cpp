#include "nes_bus.hpp"

#include <util/logging.hpp>

#include "nes_cartridge.hpp"

namespace nesem
{
	U8 NesBus::cpu_read(U16 addr) noexcept
	{
		CHECK(addr < size(ram), "Address out of range");

		if (cartridge)
		{
			auto data = cartridge->cpu_read(addr);
			if (data)
				return *data;
		}

		return ram[addr] & 255;
	}

	void NesBus::cpu_write(U16 addr, U8 value) noexcept
	{
		CHECK(addr < size(ram), "Address out of range");

		if (cartridge && cartridge->cpu_write(addr, value))
			return;

		ram[addr] = value & 255;

		if ((addr == 2 || addr == 3) && value != 0)
			DEBUG_BREAK();
	}

	void NesBus::load_cartridge(NesCartridge *cart) noexcept
	{
		this->cartridge = cart;
	}
}
