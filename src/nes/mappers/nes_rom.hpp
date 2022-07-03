#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "../nes_types.hpp"

namespace nesem::mappers
{
	enum class MirroringMode : U8
	{
		horizontal,
		vertical,
		one_screen,
		four_screen,
	};

	constexpr std::string_view to_string(MirroringMode mode) noexcept
	{
		using enum MirroringMode;
		switch (mode)
		{
		case four_screen:
			return "four-screen";
		case one_screen:
			return "one-screen";
		case horizontal:
			return "horizontal";
		case vertical:
			return "vertical";
		}

		return "*** UNKNOWN ***";
	}

	namespace ines_2
	{
		struct PrgRom
		{
			size_t size;
			std::string crc32;
			std::string sha1;
			std::string sum16;
		};

		struct ChrRom
		{
			size_t size;
			std::string crc32;
			std::string sha1;
			std::string sum16;
		};

		struct Rom
		{
			size_t size;
			std::string crc32;
			std::string sha1;
		};

		struct Pcb
		{
			int mapper;
			int submapper;
			MirroringMode mirroring;
			bool battery;
		};

		struct Console
		{
			int type;
			int region;
		};

		struct Expansion
		{
			int type;
		};

		struct ChrRam
		{
			size_t size;
		};

		struct PrgNvram
		{
			size_t size;
		};

		struct PrgRam
		{
			size_t size;
		};

		struct MiscRom
		{
			size_t size;
			std::string crc32;
			std::string sha1;
			int number;
		};

		struct Vs
		{
			int hardware;
			int ppu;
		};

		struct ChrNvram
		{
			size_t size;
		};

		struct Trainer
		{
			size_t size;
			std::string crc32;
			std::string sha1;
		};

		struct RomData
		{
			PrgRom prgrom;
			Rom rom;
			Pcb pcb;
			Console console;
			Expansion expansion;

			std::optional<ChrRom> chrrom;
			std::optional<ChrRam> chrram;
			std::optional<PrgNvram> prgnvram;
			std::optional<PrgRam> prgram;
			std::optional<MiscRom> miscrom;
			std::optional<Vs> vs;
			std::optional<ChrNvram> chrnvram;
			std::optional<Trainer> trainer;
		};

		std::string_view expansion_device_name(Expansion expansion) noexcept;
	}

	namespace ines_1
	{
		struct RomData
		{
			// iNES mapper id, see: https://wiki.nesdev.org/w/index.php?title=Mapper
			int mapper;

			// nametable mirroring mode
			MirroringMode mirroring;

			// size in 16K units
			int prg_rom_size;

			// size in 8K units
			int chr_rom_size;

			// has battery backed prg-nvram
			bool has_battery;
		};
	}

	struct NesRom
	{
		std::vector<U8> prg_rom;
		std::vector<U8> chr_rom;

		ines_1::RomData v1;
		std::optional<ines_2::RomData> v2;
	};

	enum BankSize
	{
		bank_1k = 0x0400,
		bank_2k = 0x0800,
		bank_4k = 0x1000,
		bank_8k = 0x2000,
		bank_16k = 0x4000,
		bank_32k = 0x8000,
	};

	// utility function for mappers representing physically soldered nametable maps
	// this handles one and four screen modes as selecting the first or last nametable
	// roms that provide additional nametable memory need to provide custom handling
	void apply_hardware_nametable_mapping(MirroringMode mode, U16 &addr) noexcept;
	MirroringMode mirroring_mode(const NesRom &rom) noexcept;

	int prgrom_banks(const NesRom &rom, BankSize bank_size) noexcept;
	int chrrom_banks(const NesRom &rom, BankSize bank_size) noexcept;
	int chr_banks(const NesRom &rom, BankSize bank_size) noexcept;

	bool has_chrram(const NesRom &rom) noexcept;
	size_t chrram_size(const NesRom &rom) noexcept;

	int mapper(const NesRom &rom) noexcept;

	bool has_prgram(const NesRom &rom) noexcept;
	size_t prgram_size(const NesRom &rom) noexcept;
}
