#include "nes_ppu.hpp"

#include <util/logging.hpp>

#include "nes.hpp"

namespace nesem
{
	// clang-format off
	constexpr U16 vram_fine_y_mask    = 0b0'111'00'00000'00000;
	constexpr U16 vram_nametable_mask = 0b0'000'11'00000'00000;
	constexpr U16 vram_coarse_y_mask  = 0b0'000'00'11111'00000;
	constexpr U16 vram_coarse_x_mask  = 0b0'000'00'00000'11111;
	// clang-format on

	constexpr U16 vram_fine_y_shift = 12;
	constexpr U16 vram_nametable_shift = 10;
	constexpr U16 vram_coarse_y_shift = 5;
	constexpr U16 vram_coarse_x_shift = 0;

	// clang-format off
	// NOTE: first 5 bits are unused
	constexpr U8 status_vblank          = 0b100'00000;
	constexpr U8 status_sprite0_hit     = 0b010'00000;
	constexpr U8 status_sprite_overflow = 0b001'00000;
	// clang-format on

	constexpr U8 ctrl_nmi_flag = 0b1000'0000;
	constexpr U8 ctrl_master_flag = 0b0100'0000;
	constexpr U8 ctrl_sprite_size = 0b0010'0000;
	constexpr U8 ctrl_pattern_addr = 0b0001'0000;
	constexpr U8 ctrl_sprite_addr = 0b0000'1000;
	constexpr U8 ctrl_vram_addr_inc = 0b0000'0100;
	constexpr U8 ctrl_nametable_mask = 0b0000'0011;

	NesPpu::NesPpu(Nes *nes) noexcept
		: nes(nes)
	{
		CHECK(nes != nullptr, "Nes is required");
	}

	void NesPpu::reset() noexcept
	{
		reg.ppuctrl = 0;
		reg.ppumask = 0;
		reg.ppustatus = 0;
		reg.oamaddr = 0;
		reg.addr_latch = false;
		reg.tram_addr = 0;
		reg.vram_addr = 0;

		reg.fine_x = 0;
		reg.ppudata = 0;

		framecount = 0;
		cycle = 0;
		scanline = 0;
	}

	void NesPpu::load_cartridge(NesCartridge *cart) noexcept
	{
		this->cartridge = cart;
	}

	void NesPpu::clock() noexcept
	{
	}

	U8 NesPpu::read(U16 addr) noexcept
	{
		if (cartridge)
		{
			auto value = cartridge->ppu_read(addr);
			if (value)
				return *value;
		}

		if (VERIFY(!(addr < 0x2000), "The cart should have handled this range!"))
			return 0;

		if (addr < 0x3F00)
		{
			// $2000-$23FF Nametable 0
			// $2400-$27FF Nametable 1
			// $2800-$2BFF Nametable 2
			// $2C00-$2FFF Nametable 3
			// $3000-$3EFF mirror
			// always treat nametables as vertically mirrored and let the cartridge remap addr as needed
			// TODO: This may not work for some mappers... but we'll figure it out when we get there
			return nametable[(addr >> vram_nametable_shift) & 1][addr & 0x07FF];
		}

		if (addr < 0x4000)
		{
			// palette control
			addr &= 0x1F;

			// Addresses $3F10/$3F14/$3F18/$3F1C are mirrors of $3F00/$3F04/$3F08/$3F0C.
			switch (addr)
			{
			case 0x0010:
				addr = 0x0000;
				break;
			case 0x0014:
				addr = 0x0004;
				break;
			case 0x0018:
				addr = 0x0008;
				break;
			case 0x001C:
				addr = 0x000C;
				break;
			}

			return palettes[addr];
		}

		CHECK(false, "We shouldn't get here");
		return 0;
	}

	void NesPpu::write(U16 addr, U8 value) noexcept
	{
		if (cartridge && cartridge->ppu_write(addr, value))
		{
			return;
		}

		if (VERIFY(!(addr < 0x2000), "The cart should have handled this range!"))
			return;

		if (addr < 0x3F00)
		{
			// $2000-$23FF Nametable 0
			// $2400-$27FF Nametable 1
			// $2800-$2BFF Nametable 2
			// $2C00-$2FFF Nametable 3
			// always treat nametables as vertically mirrored and let the cartridge remap addr as needed
			// TODO: This may not work for some mappers... but we'll figure it out when we get there
			nametable[(addr >> vram_nametable_shift) & 1][addr & 0x07FF] = value;
			return;
		}

		if (addr < 0x4000)
		{
			// palette control
			addr &= 0x1F;

			// Addresses $3F10/$3F14/$3F18/$3F1C are mirrors of $3F00/$3F04/$3F08/$3F0C.
			switch (addr)
			{
			case 0x0010:
				addr = 0x0000;
				break;
			case 0x0014:
				addr = 0x0004;
				break;
			case 0x0018:
				addr = 0x0008;
				break;
			case 0x001C:
				addr = 0x000C;
				break;
			}

			palettes[addr] = value;
			return;
		}

		CHECK(false, "We shouldn't get here");
	}

	U8 NesPpu::ppuctrl() noexcept
	{
		return latch;
	}

	void NesPpu::ppuctrl(U8 value) noexcept
	{
		reg.ppuctrl = latch = value;
		reg.tram_addr = (reg.tram_addr & ~vram_nametable_mask) | ((value & 3) << vram_nametable_shift);
	}

	U8 NesPpu::ppumask() noexcept
	{
		return latch;
	}

	void NesPpu::ppumask(U8 value) noexcept
	{
		reg.ppumask = latch = value;
	}

	U8 NesPpu::ppustatus() noexcept
	{
		// reading status not only returns the status register, it also resets the address latch and clears the vertical blank flag
		// the unused bits of status are filled with the latched value from other write operations
		latch = (0b11100000 & reg.ppustatus) | (0b00011111 & latch);

		reg.ppustatus &= U8(~status_vblank);
		reg.addr_latch = false;

		return latch;
	}

	void NesPpu::ppustatus(U8 value) noexcept
	{
		latch = value;
	}

	U8 NesPpu::oamaddr() noexcept
	{
		return latch;
	}

	void NesPpu::oamaddr(U8 value) noexcept
	{
		reg.oamaddr = latch = value;
	}

	U8 NesPpu::oamdata() noexcept
	{
		return oam[reg.oamaddr];
	}

	void NesPpu::oamdata(U8 value) noexcept
	{
		oam[reg.oamaddr++] = value;
	}

	U8 NesPpu::ppuscroll() noexcept
	{
		return latch;
	}

	void NesPpu::ppuscroll(U8 value) noexcept
	{
		if (!reg.addr_latch)
		{
			// write values for x
			reg.fine_x = value & 7;
			reg.tram_addr = (reg.tram_addr & ~vram_coarse_x_mask) | (value >> 3);
		}
		else
		{
			// write values for y
			reg.tram_addr =
				(reg.tram_addr & ~(vram_fine_y_mask | vram_coarse_y_mask)) |
				((value & 7) << vram_fine_y_shift) |
				((value >> 3) << vram_coarse_y_shift);
		}

		reg.addr_latch = !reg.addr_latch;
	}

	U8 NesPpu::ppuaddr() noexcept
	{
		return latch;
	}

	void NesPpu::ppuaddr(U8 value) noexcept
	{
		if (!reg.addr_latch)
		{
			reg.tram_addr = (reg.tram_addr & 0x00FF) | ((value & 0x7F) << 8);
		}
		else
		{
			reg.tram_addr = (reg.tram_addr & 0xFF00) | value;
			reg.vram_addr = reg.tram_addr;
		}

		reg.addr_latch = !reg.addr_latch;
	}

	U8 NesPpu::ppudata() noexcept
	{
		// non-palette data is buffered
		auto result = std::exchange(reg.ppudata, read(reg.vram_addr));

		// palette data is returned immediately
		if (reg.vram_addr >= 0x3F00)
			result = reg.ppudata;

		// the address is incremented on every read
		reg.vram_addr += (reg.ppuctrl & ctrl_vram_addr_inc ? 32 : 1);

		return result;
	}

	void NesPpu::ppudata(U8 value) noexcept
	{
		write(reg.vram_addr, value);

		// the address is incremented on every write
		reg.vram_addr += (reg.ppuctrl & ctrl_vram_addr_inc ? 32 : 1);
	}
}
