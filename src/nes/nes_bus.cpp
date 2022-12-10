#include "nes_bus.hpp"

#include "nes.hpp"
#include "nes_cartridge.hpp"

#include <util/logging.hpp>

namespace nesem
{
	NesBus::NesBus(Nes *nes) noexcept
		: nes(nes)
	{
		CHECK(nes != nullptr, "Nes should not be null!");
	}

	void NesBus::clock() noexcept
	{
		if (cartridge)
			cartridge->signal_m2(false);
	}

	U8 NesBus::peek(U16 addr) const noexcept
	{
		if (addr < 0x2000)
			return ram[addr & 0x7FF] & 255;

		// this range maps to PPU registers
		if (addr < 0x4000)
		{
			LOG_WARN("Peek of PPU registers ignored for addr ${:04X}", addr);
			return 0;
		}

		if (addr < 0x6000)
		{
			// no peek for input, apu, and other stuff
			LOG_WARN("Peek ignored for addr ${:04X}", addr);
			return 0;
		}

		// 0x4020 - 0xFFFF go to the cart
		if (cartridge)
			return cartridge->cpu_peek(addr);
		else
		{
			LOG_WARN("no cartridge, ignoring peek for ${:04X}", addr);
			return 0;
		}

		CHECK(false, "We shouldn't get here");
		return 0;
	}

	U8 NesBus::read(U16 addr, NesBusOp op) noexcept
	{
		// cartridge can observe all reads. This allows e.g. MMC3 to observe m2 changes and MMC5 to mirror PPU state
		if (cartridge)
			last_read_value = cartridge->cpu_read(addr);

		// the NES has 2k of ram mirrored 4 times between addr 0 - 0x1FFF
		// thus, reads/writes to 0x0001 are observable at address 0x0801, 0x1001, 0x1801
		if (addr < 0x2000)
			last_read_value = ram[addr & 0x7FF] & 255;

		// this range maps to PPU registers
		else if (addr < 0x4000)
		{
			if (op == NesBusOp::ready)
			{
				switch (addr & 7)
				{
				case 0: // PPUCTRL
					last_read_value = nes->ppu().ppuctrl();
					break;

				case 1: // PPUMASK
					last_read_value = nes->ppu().ppumask();
					break;

				case 2: // PPUSTATUS
					last_read_value = nes->ppu().ppustatus();
					break;

				case 3: // OAMADDR
					last_read_value = nes->ppu().oamaddr();
					break;

				case 4: // OAMDATA
					last_read_value = nes->ppu().oamdata();
					break;

				case 5: // PPUSCROLL
					last_read_value = nes->ppu().ppuscroll();
					break;

				case 6: // PPUADDR
					last_read_value = nes->ppu().ppuaddr();
					break;

				case 7: // PPUDATA
					last_read_value = nes->ppu().ppudata();
					break;
				}
			}
		}

		// read controller data
		else if (addr == 0x4016 || addr == 0x4017)
		{
			// if we are in poll mode, make sure to poll. This has the effect of returning the A button over and over again
			// I don't know if any games actually make use of this, but that is how the hardware behaves
			if (poll_input)
			{
				nes->player1().poll();
				nes->player2().poll();
			}

			if (addr == 0x4016)
				last_read_value = nes->player1().read();
			else
				last_read_value = nes->player2().read();
		}

		// NES APU and I/O registers
		else if (addr < 0x4018)
			last_read_value = nes->apu().read(addr);

		// unused area, intended for hardware testing and APU features, but disabled on commercial NES units
		else if (addr < 0x4020)
			LOG_WARN("disabled address, ignoring read from ${:04X}", addr);

		return last_read_value;
	}

	void NesBus::write(U16 addr, U8 value, NesBusOp op) noexcept
	{
		// cartridge can observe all writes. This allows e.g. MMC3 to observe m2 changes and MMC5 to mirror PPU state
		if (cartridge)
			cartridge->cpu_write(addr, value);

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
			if (op == NesBusOp::ready)
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
				nes->player1().poll();
				nes->player2().poll();
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
	}

	void NesBus::load_cartridge(NesCartridge *cart) noexcept
	{
		this->cartridge = cart;
	}

	U8 NesBus::open_bus_read() const noexcept
	{
		return last_read_value;
	}
}
