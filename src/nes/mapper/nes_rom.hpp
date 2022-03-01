#pragma once

#include <filesystem>
#include <optional>
#include <vector>

#include "../nes_types.hpp"

namespace nesem::mapper
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

	// utility function for mappers representing physically soldered nametable maps
	void apply_hardware_nametable_mapping(const NesRom &rom, U16 &addr) noexcept;
}
