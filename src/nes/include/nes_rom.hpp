#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <nes_types.hpp>

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

		enum class Expansion
		{
			unspecified = 0x00,
			standard_controller = 0x01,
			four_score = 0x02,
			famicom_four_player = 0x03,
			vs_system = 0x04,
			vs_system_reversed_inputs = 0x05,
			vs_pinball = 0x06,
			vs_zapper = 0x07,
			zapper = 0x08,
			two_zappers = 0x09,
			bandai_hyper_shot = 0x0A,
			power_pad_side_a = 0x0B,
			power_pad_side_b = 0x0C,
			family_trainer_side_a = 0x0D,
			family_trainer_side_b = 0x0E,
			arkanoid_vaus_nes = 0x0F,
			arkanoid_vaus_famicom = 0x10,
			two_vaus = 0x11,
			konami_hyper_shot = 0x12,
			coconuts_pachinko = 0x13,
			punching_bag = 0x14,
			jissen_mahjong = 0x15,
			party_tap = 0x16,
			oeka_kids_tablet = 0x17,
			sunsoft_barcode_battler = 0x18,
			miracle_piano = 0x19,
			pokkun_moguraa = 0x1A,
			top_rider = 0x1B,
			double_fisted = 0x1C,
			famicom_3d = 0x1D,
			doremikko_keyboard = 0x1E,
			rob_gyro_set = 0x1F,
			famicom_data_recorder = 0x20,
			ascii_turbo_file = 0x21,
			igs_storage_battle_box = 0x22,
			family_basic_keyboard = 0x23,
			dongda_pec_586 = 0x24,
			bit_79 = 0x25,
			subor_keyboard = 0x26,
			subor_keyboard_3x8_bit = 0x27,
			subor_keyboard_24_bit = 0x28,
			snes_mouse = 0x29,
			multicart = 0x2A,
			two_snes_controllers = 0x2B,
			racermate_bicycle = 0x2C,
			u_force = 0x2D,
			rob_stack_up = 0x2E,
			city_patrolman_lightgun = 0x2F,
			sharp_c1_cassette = 0x30,
			standard_controller_swapped = 0x31,
			excalibor_sudoku_pad = 0x32,
			abl_pinball = 0x33,
			golden_nugget_casino = 0x34,
		};

		using ChrRam = size_t;
		using PrgNvram = size_t;
		using PrgRam = size_t;
		using ChrNvram = size_t;

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
			// ines version from file
			int version;

			// iNES mapper id, see: https://wiki.nesdev.org/w/index.php?title=Mapper
			int mapper;

			// nametable mirroring mode
			MirroringMode mirroring;

			// size in 16K units
			U8 prg_rom_size;

			// size in 8K units
			U8 chr_rom_size;

			// size in 8K units
			U8 prg_ram_size;

			// has battery backed prg-nvram
			bool has_battery;

			// has 512k trainer data
			bool has_trainer;

			// optional INST-ROM, 8192 bytes if present
			bool has_inst_rom;
		};
	}

	struct NesRom
	{
		std::vector<U8> prg_rom;
		std::vector<U8> chr_rom;
		std::string sha1;

		ines_1::RomData v1;
		std::optional<ines_2::RomData> v2;
	};

	// utility function for mappers representing physically soldered nametable maps
	// this handles one and four screen modes as selecting the first or last nametable
	// roms that provide additional nametable memory need to provide custom handling
	void apply_hardware_nametable_mapping(MirroringMode mode, Addr &addr) noexcept;
	MirroringMode rom_mirroring_mode(const NesRom &rom) noexcept;

	U32 rom_prgrom_banks(const NesRom &rom, BankSize bank_size) noexcept;
	U32 rom_chrrom_banks(const NesRom &rom, BankSize bank_size) noexcept;
	U32 rom_chr_banks(const NesRom &rom, BankSize bank_size) noexcept;

	bool rom_has_chrram(const NesRom &rom) noexcept;
	size_t rom_chrram_size(const NesRom &rom) noexcept;

	int rom_mapper(const NesRom &rom) noexcept;

	bool rom_has_prgram(const NesRom &rom) noexcept;
	size_t rom_prgram_size(const NesRom &rom) noexcept;

	bool rom_has_bus_conflicts(const NesRom &rom) noexcept;

	int rom_region(const NesRom &rom) noexcept;

	// map a system address to an address on the rom.
	size_t to_rom_addr(size_t bank, size_t bank_size, Addr addr) noexcept;
}
