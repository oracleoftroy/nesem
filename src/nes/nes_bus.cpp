#include "nes_bus.hpp"

#include <util/logging.hpp>

#include "nes.hpp"
#include "nes_cartridge.hpp"

namespace nesem
{
	NesBus::NesBus(Nes *nes) noexcept
		: nes(nes)
	{
		CHECK(nes != nullptr, "Nes should not be null!");
	}

	U8 NesBus::read(U16 addr) noexcept
	{
		// the NES has 2k of ram mirrored 4 times between addr 0 - 0x1FFF
		// thus, reads/writes to 0x0001 are observable at address 0x0801, 0x1001, 0x1801
		if (addr < 0x2000)
			return ram[addr & 0x7FF] & 255;

		// this range maps to PPU registers
		if (addr < 0x4000)
		{
			switch (addr & 7)
			{
			case 0: // PPUCTRL
				return nes->ppu().ppuctrl();

			case 1: // PPUMASK
				return nes->ppu().ppumask();

			case 2: // PPUSTATUS
				return nes->ppu().ppustatus();

			case 3: // OAMADDR
				return nes->ppu().oamaddr();

			case 4: // OAMDATA
				return nes->ppu().oamdata();

			case 5: // PPUSCROLL
				return nes->ppu().ppuscroll();

			case 6: // PPUADDR
				return nes->ppu().ppuaddr();

			case 7: // PPUDATA
				return nes->ppu().ppudata();
			}
		}

		// read controller data
		if (addr == 0x4016 || addr == 0x4017)
		{
			// if we are in poll mode, make sure to poll. This has the effect of returning the A button over and over again
			// I don't know if any games actually make use of this, but that is how the hardware behaves
			if (poll_input)
			{
				controller1 = nes->poll_player1();
				controller2 = nes->poll_player2();
			}

			// return the next button and shift our latched data one bit. 1 for button down, 0 for up. Offical controllers will return 1 if
			auto read_controller_next = [](U8 &v) {
				// Open bus behavior will write the high 3 bits of the address into bits 5-7. Some games (e.g. paperboy) rely on this to detect buttons
				// see: https://wiki.nesdev.org/w/index.php?title=Open_bus_behavior
				U8 result = 0x40 | (v & 1);
				v >>= 1;
				v |= 0b10000000;

				return result;
			};

			if (addr == 0x4016)
				return read_controller_next(controller1);
			else
				return read_controller_next(controller2);
		}

		// NES APU and I/O registers
		if (addr < 0x4018)
			return nes->apu().read(addr);

		// unused area, intended for hardware testing and APU features, but disabled on commercial NES units
		if (addr < 0x4020)
		{
			LOG_WARN("disabled address, ignoring read from ${:04X}", addr);
			return 0;
		}

		// 0x4020 - 0xFFFF go to the cart
		if (cartridge)
			return cartridge->cpu_read(addr);
		else
		{
			LOG_WARN("no cartridge, ignoring read from ${:04X}", addr);
			return 0;
		}

		CHECK(false, "We shouldn't get here");
		return 0;
	}

	void NesBus::write(U16 addr, U8 value) noexcept
	{
		// the NES has 2k of ram mirrored 4 times between addr 0 - 0x1FFF
		// thus, reads/writes to 0x0001 are observable at address 0x0801, 0x1001, 0x1801
		if (addr < 0x2000)
		{
			ram[addr & 0x7FF] = value & 255;
			return;
		}

		// this range maps to PPU registers
		if (addr < 0x4000)
		{
			switch (addr & 7)
			{
			case 0: // PPUCTRL
				nes->ppu().ppuctrl(value);
				break;
			case 1: // PPUMASK
				nes->ppu().ppumask(value);
				break;
			case 2: // PPUSTATUS
				nes->ppu().ppustatus(value);
				break;
			case 3: // OAMADDR
				nes->ppu().oamaddr(value);
				break;
			case 4: // OAMDATA
				nes->ppu().oamdata(value);
				break;
			case 5: // PPUSCROLL
				nes->ppu().ppuscroll(value);
				break;
			case 6: // PPUADDR
				nes->ppu().ppuaddr(value);
				break;
			case 7: // PPUDATA
				nes->ppu().ppudata(value);
				break;
			}

			return;
		}

		if (addr == 0x4014)
		{
			// OAM DMA
			nes->cpu().dma(value);
			return;
		}

		// toggle controller polling
		if (addr == 0x4016)
		{
			// poll if we are currently in a polling mode before resetting the value. This way we get the most up to date value before a normal read mode.
			// It doesn't matter if we were already polling and this write keeps us in a poll mode
			if (poll_input)
			{
				controller1 = nes->poll_player1();
				controller2 = nes->poll_player2();
			}

			poll_input = (value & 1) == 1;
			return;
		}

		// NES APU and I/O registers
		if (addr < 0x4018)
		{
			nes->apu().write(addr, value);
			return;
		}

		// unused area, intended for hardware testing and APU features, but disabled on commercial NES units
		if (addr < 0x4020)
		{
			LOG_WARN("disabled address, ignoring write to ${:04X}", addr);
			return;
		}

		// 0x4020 - 0xFFFF go to the cart
		if (cartridge)
		{
			cartridge->cpu_write(addr, value);
			return;
		}
		else
		{
			LOG_WARN("no cartridge, ignoring write to ${:04X}", addr);
			return;
		}

		CHECK(false, "We shouldn't get here");
	}

	void NesBus::load_cartridge(NesCartridge *cart) noexcept
	{
		this->cartridge = cart;
	}
}
