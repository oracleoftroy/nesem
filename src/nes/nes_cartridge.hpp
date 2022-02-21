#pragma once

#include <filesystem>
#include <optional>
#include <vector>

#include "nes_types.hpp"

namespace nesem
{
	enum class NesMirroring
	{
		horizontal,
		vertical,
	};

	struct NesRom
	{
		// iNES file version
		int version;

		// iNES mapper id, see: https://wiki.nesdev.org/w/index.php?title=Mapper
		int mapper;

		// nametable mirroring mode
		NesMirroring mirroring;

		// indicates special handling of mirroring, exact handling varies by mapper
		bool mirror_override;

		// size in 16K units
		int prg_rom_size;

		// size in 8K units
		int chr_rom_size;

		std::vector<U8> prg_rom;
		std::vector<U8> chr_rom;
	};

	std::optional<NesRom> read_rom(const std::filesystem::path &filename) noexcept;

	class NesCartridge final
	{
	public:
		explicit NesCartridge(const NesRom &rom) noexcept;

		U8 cpu_read(U16 addr) noexcept;
		void cpu_write(U16 addr, U8 value) noexcept;

		// read data from chr_rom. The cartridge has a lot of leeway to remap the PPU
		// if return is not nullopt, the cart handled the request
		// if return is nullopt, the ppu should handle the request directly. Note that the cartridge may freely modify the address.
		std::optional<U8> ppu_read(U16 &addr) noexcept;

		// write data from chr_rom. The cartridge has a lot of leeway to remap the PPU
		// if return is true, the cart handled the request
		// if return is false, the ppu should handle the request directly. Note that the cartridge may freely modify the address.
		bool ppu_write(U16 &addr, U8 value) noexcept;

	private:
		NesRom rom;
	};
}
