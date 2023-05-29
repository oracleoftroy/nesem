#include "nes_rom.hpp"

#include <util/logging.hpp>

namespace nesem::mappers::ines_2
{
	std::string_view expansion_device_name(Expansion expansion) noexcept
	{
		using namespace std::string_view_literals;
		using enum Expansion;

		// names from https://www.nesdev.org/wiki/NES_2.0#Default_Expansion_Device
		switch (expansion)
		{
		default:
			return "Unknown device"sv;
		case unspecified:
			return "Unspecified"sv;
		case standard_controller:
			return "Standard NES/Famicom controllers"sv;
		case four_score:
			return "NES Four Score/Satellite with two additional standard controllers"sv;
		case famicom_four_player:
			return "Famicom Four Players Adapter with two additional standard controllers"sv;
		case vs_system:
			return "Vs. System"sv;
		case vs_system_reversed_inputs:
			return "Vs. System with reversed inputs"sv;
		case vs_pinball:
			return "Vs. Pinball (Japan)"sv;
		case vs_zapper:
			return "Vs. Zapper"sv;
		case zapper:
			return "Zapper ($4017)"sv;
		case two_zappers:
			return "Two Zappers"sv;
		case bandai_hyper_shot:
			return "Bandai Hyper Shot Lightgun"sv;
		case power_pad_side_a:
			return "Power Pad Side A"sv;
		case power_pad_side_b:
			return "Power Pad Side B"sv;
		case family_trainer_side_a:
			return "Family Trainer Side A"sv;
		case family_trainer_side_b:
			return "Family Trainer Side B"sv;
		case arkanoid_vaus_nes:
			return "Arkanoid Vaus Controller (NES)"sv;
		case arkanoid_vaus_famicom:
			return "Arkanoid Vaus Controller (Famicom)"sv;
		case two_vaus:
			return "Two Vaus Controllers plus Famicom Data Recorder"sv;
		case konami_hyper_shot:
			return "Konami Hyper Shot Controller"sv;
		case coconuts_pachinko:
			return "Coconuts Pachinko Controller"sv;
		case punching_bag:
			return "Exciting Boxing Punching Bag (Blowup Doll)"sv;
		case jissen_mahjong:
			return "Jissen Mahjong Controller"sv;
		case party_tap:
			return "Party Tap"sv;
		case oeka_kids_tablet:
			return "Oeka Kids Tablet"sv;
		case sunsoft_barcode_battler:
			return "Sunsoft Barcode Battler"sv;
		case miracle_piano:
			return "Miracle Piano Keyboard"sv;
		case pokkun_moguraa:
			return "Pokkun Moguraa (Whack-a-Mole Mat and Mallet)"sv;
		case top_rider:
			return "Top Rider (Inflatable Bicycle)"sv;
		case double_fisted:
			return "Double-Fisted (Requires or allows use of two controllers by one player)"sv;
		case famicom_3d:
			return "Famicom 3D System"sv;
		case doremikko_keyboard:
			return "Doremikko Keyboard"sv;
		case rob_gyro_set:
			return "R.O.B. Gyro Set"sv;
		case famicom_data_recorder:
			return "Famicom Data Recorder (don't emulate keyboard)"sv;
		case ascii_turbo_file:
			return "ASCII Turbo File"sv;
		case igs_storage_battle_box:
			return "IGS Storage Battle Box"sv;
		case family_basic_keyboard:
			return "Family BASIC Keyboard plus Famicom Data Recorder"sv;
		case dongda_pec_586:
			return "Dongda PEC-586 Keyboard"sv;
		case bit_79:
			return "Bit Corp. Bit-79 Keyboard"sv;
		case subor_keyboard:
			return "Subor Keyboard"sv;
		case subor_keyboard_3x8_bit:
			return "Subor Keyboard plus mouse (3x8-bit protocol)"sv;
		case subor_keyboard_24_bit:
			return "Subor Keyboard plus mouse (24-bit protocol)"sv;
		case snes_mouse:
			return "SNES Mouse (case 0x40:17.d0)"sv;
		case multicart:
			return "Multicart"sv;
		case two_snes_controllers:
			return "Two SNES controllers replacing the two standard NES controllers"sv;
		case racermate_bicycle:
			return "RacerMate Bicycle"sv;
		case u_force:
			return "U-Force"sv;
		case rob_stack_up:
			return "R.O.B. Stack-Up"sv;
		case city_patrolman_lightgun:
			return "City Patrolman Lightgun"sv;
		case sharp_c1_cassette:
			return "Sharp C1 Cassette Interface"sv;
		case standard_controller_swapped:
			return "Standard Controller with swapped Left-Right/Up-Down/B-A"sv;
		case excalibor_sudoku_pad:
			return "Excalibor Sudoku Pad"sv;
		case abl_pinball:
			return "ABL Pinball"sv;
		case golden_nugget_casino:
			return "Golden Nugget Casino extra buttons"sv;
		}
	}

}

namespace nesem::mappers
{
	template <typename R>
	constexpr R checked_cast(auto value) noexcept
	{
		if (std::is_constant_evaluated())
		{
			if (std::in_range<R>(value))
				return static_cast<R>(value);
		}
		else
		{
			CHECK(std::in_range<R>(value), "value out of range");
			return static_cast<R>(value);
		}
	}

	void apply_hardware_nametable_mapping(MirroringMode mode, Addr &addr) noexcept
	{
		using enum MirroringMode;

		switch (mode)
		{
		case horizontal:
			// exchange the nt_x and nt_y bits
			// we assume vertical mirroring by default, so this flips the 2400-27ff
			// range with the 2800-2BFF range to achieve a horizontal mirror
			addr = (addr & ~0b0'000'11'00000'00000) |
				((addr & 0b0'000'10'00000'00000) >> 1) |
				((addr & 0b0'000'01'00000'00000) << 1);
			break;

		case vertical:
			// nothing to do
			break;

		case one_screen:
			// force address to first nametable
			addr = (addr & ~0b0'000'11'00000'00000);
			break;

		case four_screen:
			// force address to last nametable
			addr = (addr & ~0b0'000'11'00000'00000) | (3 << 10);
			break;
		}
	}

	MirroringMode rom_mirroring_mode(const NesRom &rom) noexcept
	{
		if (rom.v2)
			return rom.v2->pcb.mirroring;

		return rom.v1.mirroring;
	}

	U32 rom_prgrom_banks(const NesRom &rom, BankSize bank_size) noexcept
	{
		if (rom.v2)
			return checked_cast<U32>(rom.v2->prgrom.size / bank_size);

		return (rom.v1.prg_rom_size * bank_16k) / bank_size;
	}

	U32 rom_chrrom_banks(const NesRom &rom, BankSize bank_size) noexcept
	{
		if (rom.v2)
			return rom.v2->chrrom.has_value() ? checked_cast<U32>(rom.v2->chrrom->size / bank_size) : 0;

		return (rom.v1.chr_rom_size * bank_8k) / bank_size;
	}

	U32 rom_chr_banks(const NesRom &rom, BankSize bank_size) noexcept
	{
		if (rom_has_chrram(rom))
			return checked_cast<U32>(rom_chrram_size(rom) / bank_size);

		return rom_chrrom_banks(rom, bank_size);
	}

	bool rom_has_chrram(const NesRom &rom) noexcept
	{
		if (rom.v2)
			return rom.v2->chrram.has_value();

		return rom.v1.chr_rom_size == 0;
	}

	size_t rom_chrram_size(const NesRom &rom) noexcept
	{
		if (rom.v2)
			return rom.v2->chrram.value_or(0);

		return rom.v1.chr_rom_size == 0 ? bank_8k : 0;
	}

	int rom_mapper(const NesRom &rom) noexcept
	{
		if (rom.v2)
			return rom.v2->pcb.mapper;

		return rom.v1.mapper;
	}

	bool rom_has_prgram(const NesRom &rom) noexcept
	{
		// TODO: should we report true if there is a battery but not prgnvram or chrnvram?
		// I see 18 entries in nesdb that have a battery but don't report any sort of ram at all
		// and 104 without nv ram, but have some prg or chr ram
		if (rom.v2)
			return rom.v2->prgram.has_value() || rom.v2->prgnvram.has_value();

		return rom.v1.has_battery;
	}

	size_t rom_prgram_size(const NesRom &rom) noexcept
	{
		size_t size = 0;

		if (rom.v2)
		{
			size += rom.v2->prgram.value_or(0);
			size += rom.v2->prgnvram.value_or(0);
		}
		else if (rom.v1.has_battery)
			size = bank_8k;

		return size;
	}

	bool rom_has_bus_conflicts(const NesRom &rom) noexcept
	{
		// https://www.nesdev.org/wiki/Category:Mappers_with_bus_conflicts
		// INES Mapper 003
		// INES Mapper 034
		// INES Mapper 152
		// INES Mapper 185
		// INES Mapper 188
		// AxROM (Mapper 007)
		// BNROM (Mapper 034)
		// Color Dreams (Mapper 011)
		// CPROM (Mapper 013)
		// GxROM (Mapper 066)
		// UNROM 512 (Mapper 030)
		// UxROM (Mapper 002, 094, 180)

		if (rom.v2)
		{
			// iNES 2 Mappers 2, 3, and 7 designate submappers for whether the cart has bus conflicts
			// From https://www.nesdev.org/wiki/NES_2.0_submappers#002,_003,_007:_UxROM,_CNROM,_AxROM

			// 0: Default iNES behaviour (Emulators should warn the user and/or enforce bus conflicts for mappers 2 and 3, and should warn the user and/or fail to enforce bus conflicts for mapper 7)
			// 1: Bus conflicts do not occur
			// 2: Bus conflicts occur, producing the bitwise AND of the written value and the value in ROM

			switch (rom.v2->pcb.mapper)
			{
			case 2:
			case 3:
				if (rom.v2->pcb.submapper == 0)
					return true;

				[[fallthrough]];
			case 7:
				return rom.v2->pcb.submapper == 2;

			case 34:
				// iNES 2 submapper distinguishes the NINA-001 (no conflicts) and BNROM (conflicts), so use that if we have it

				// NINA-001 - always no
				if (rom.v2->pcb.submapper == 1)
					return false;

				// BNROM - always yes
				if (rom.v2->pcb.submapper == 2)
					return true;
			}
		}

		switch (rom_mapper(rom))
		{
		case 34:
			// We did not have a more precise submapper, either because it is 0 or because we only have iNES 1 info

			// From: https://www.nesdev.org/wiki/NES_2.0_submappers#034:_BNROM_/_NINA-001

			// To disambiguate the two mappers, emulators have taken various approaches:
			// The presence of CHR larger than 8 KiB unambiguously requires NINA-001, as BNROM has no CHR banking.
			if (rom_chr_banks(rom, bank_8k) > 1)
				return false;

			// The presence of CHR-RAM is taken to imply BNROM, because both extant BNROM games use CHR-RAM.
			if (rom_has_chrram(rom))
				return true;

			// All explicit and implicit heuristics failed, we'll err on the side of no conflicts
			return false;

			// assume these all have bus conflicts if we got to this point
		case 2:
		case 3:
		case 11:
		case 13:
		case 30:
		case 66:
		case 94:
		case 152:
		case 180:
		case 185:
		case 188:
			return true;
		}

		return false;
	}

	int rom_region(const NesRom &rom) noexcept
	{
		if (rom.v2)
			return rom.v2->console.region;

		// nesdev wiki indicates that this field was not widely used in ines 1 headers, so just default to JP/US
		return 0;
	}

	// map a system address to an address on the rom.
	size_t to_rom_addr(size_t bank, size_t bank_size, Addr addr) noexcept
	{
		CHECK(std::has_single_bit(bank_size), "banks must be a power of two");
		return bank * bank_size + to_integer(addr & (bank_size - 1));
	}
}
