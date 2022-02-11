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
		CHECK(addr < size(ram), "Address out of range");

		// cartridge is 0x4020-0xFFFF, but we always go to the cart first to let mappers possibly adjust the address
		if (cartridge)
			return cartridge->cpu_read(addr);

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

		// NES APU and I/O registers
		if (addr < 0x4018)
		{
			LOG_WARN("APU not implemented, ignoring read");
			return 0;
		}

		// unused area, intended for hardware testing and APU features, but disabled on commercial NES units
		if (addr < 0x4020)
		{
			LOG_WARN("disabled address, ignoring read");
			return 0;
		}

		// 0x4020 - 0xFFFF go to the cart
		if (cartridge)
			return cartridge->cpu_read(addr);
		else
		{
			LOG_WARN("no cartridge, ignoring read");
			return 0;
		}

		CHECK(false, "We shouldn't get here");
		return 0;
	}

	void NesBus::write(U16 addr, U8 value) noexcept
	{
		CHECK(addr < size(ram), "Address out of range");

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

		// NES APU and I/O registers
		if (addr < 0x4018)
		{
			LOG_WARN("APU not implemented, ignoring write");
			return;
		}

		// unused area, intended for hardware testing and APU features, but disabled on commercial NES units
		if (addr < 0x4020)
		{
			LOG_WARN("disabled address, ignoring write");
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
			LOG_WARN("no cartridge, ignoring write");
			return;
		}

		CHECK(false, "We shouldn't get here");
	}

	void NesBus::load_cartridge(NesCartridge *cart) noexcept
	{
		this->cartridge = cart;
	}
}
