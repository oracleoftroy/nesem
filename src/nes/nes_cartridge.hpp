#pragma once

#include <filesystem>
#include <optional>

#include "nes_types.hpp"

namespace nesem
{
	class NesCartridge
	{
	public:
		virtual void reset() noexcept = 0;

		virtual U8 cpu_read(U16 addr) noexcept = 0;
		virtual void cpu_write(U16 addr, U8 value) noexcept = 0;

		// read data from chr_rom. The cartridge has a lot of leeway to remap the PPU
		// if return is not nullopt, the cart handled the request
		// if return is nullopt, the ppu should handle the request directly. Note that the cartridge may freely modify the address.
		virtual std::optional<U8> ppu_read(U16 &addr) noexcept = 0;

		// write data from chr_rom. The cartridge has a lot of leeway to remap the PPU
		// if return is true, the cart handled the request
		// if return is false, the ppu should handle the request directly. Note that the cartridge may freely modify the address.
		virtual bool ppu_write(U16 &addr, U8 value) noexcept = 0;
	};

	std::unique_ptr<NesCartridge> load_cartridge(const std::filesystem::path &filename) noexcept;
}
