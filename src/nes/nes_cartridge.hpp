#pragma once

#include <filesystem>
#include <optional>
#include <vector>

#include "nes_types.hpp"

namespace nesem
{
	struct NesRom
	{
		int version;
		int mapper;

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

		// attempt a read of the cartridge.
		// if the cart handles it, returns byte read, else returns std::nullopt
		std::optional<U8> cpu_read(U16 addr) noexcept;

		// attempt a write to the cartridge.
		// if the cart handles it, returns true, else returns false
		bool cpu_write(U16 addr, U8 value) noexcept;

	private:
		NesRom rom;
	};
}
