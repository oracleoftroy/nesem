#include "nes_rom.hpp"

namespace nesem::mappers::ines_2
{
	std::string_view expansion_device_name(Expansion expansion) noexcept
	{
		using namespace std::string_view_literals;
		// names from https://www.nesdev.org/wiki/NES_2.0#Default_Expansion_Device
		switch (expansion.type)
		{
		default:
			return "Unknown device"sv;
		case 0x00:
			return "Unspecified"sv;
		case 0x01:
			return "Standard NES/Famicom controllers"sv;
		case 0x02:
			return "NES Four Score/Satellite with two additional standard controllers"sv;
		case 0x03:
			return "Famicom Four Players Adapter with two additional standard controllers"sv;
		case 0x04:
			return "Vs. System"sv;
		case 0x05:
			return "Vs. System with reversed inputs"sv;
		case 0x06:
			return "Vs. Pinball (Japan)"sv;
		case 0x07:
			return "Vs. Zapper"sv;
		case 0x08:
			return "Zapper ($4017)"sv;
		case 0x09:
			return "Two Zappers"sv;
		case 0x0A:
			return "Bandai Hyper Shot Lightgun"sv;
		case 0x0B:
			return "Power Pad Side A"sv;
		case 0x0C:
			return "Power Pad Side B"sv;
		case 0x0D:
			return "Family Trainer Side A"sv;
		case 0x0E:
			return "Family Trainer Side B"sv;
		case 0x0F:
			return "Arkanoid Vaus Controller (NES)"sv;
		case 0x10:
			return "Arkanoid Vaus Controller (Famicom)"sv;
		case 0x11:
			return "Two Vaus Controllers plus Famicom Data Recorder"sv;
		case 0x12:
			return "Konami Hyper Shot Controller"sv;
		case 0x13:
			return "Coconuts Pachinko Controller"sv;
		case 0x14:
			return "Exciting Boxing Punching Bag (Blowup Doll)"sv;
		case 0x15:
			return "Jissen Mahjong Controller"sv;
		case 0x16:
			return "Party Tap "sv;
		case 0x17:
			return "Oeka Kids Tablet"sv;
		case 0x18:
			return "Sunsoft Barcode Battler"sv;
		case 0x19:
			return "Miracle Piano Keyboard"sv;
		case 0x1A:
			return "Pokkun Moguraa (Whack-a-Mole Mat and Mallet)"sv;
		case 0x1B:
			return "Top Rider (Inflatable Bicycle)"sv;
		case 0x1C:
			return "Double-Fisted (Requires or allows use of two controllers by one player)"sv;
		case 0x1D:
			return "Famicom 3D System"sv;
		case 0x1E:
			return "Doremikko Keyboard"sv;
		case 0x1F:
			return "R.O.B. Gyro Set"sv;
		case 0x20:
			return "Famicom Data Recorder (don't emulate keyboard)"sv;
		case 0x21:
			return "ASCII Turbo File"sv;
		case 0x22:
			return "IGS Storage Battle Box"sv;
		case 0x23:
			return "Family BASIC Keyboard plus Famicom Data Recorder"sv;
		case 0x24:
			return "Dongda PEC-586 Keyboard"sv;
		case 0x25:
			return "Bit Corp. Bit-79 Keyboard"sv;
		case 0x26:
			return "Subor Keyboard"sv;
		case 0x27:
			return "Subor Keyboard plus mouse (3x8-bit protocol)"sv;
		case 0x28:
			return "Subor Keyboard plus mouse (24-bit protocol)"sv;
		case 0x29:
			return "SNES Mouse (case 0x40:17.d0)"sv;
		case 0x2A:
			return "Multicart"sv;
		case 0x2B:
			return "Two SNES controllers replacing the two standard NES controllers"sv;
		case 0x2C:
			return "RacerMate Bicycle"sv;
		case 0x2D:
			return "U-Force"sv;
		case 0x2E:
			return "R.O.B. Stack-Up"sv;
		case 0x2F:
			return "City Patrolman Lightgun"sv;
		case 0x30:
			return "Sharp C1 Cassette Interface"sv;
		case 0x31:
			return "Standard Controller with swapped Left-Right/Up-Down/B-A"sv;
		case 0x32:
			return "Excalibor Sudoku Pad"sv;
		case 0x33:
			return "ABL Pinball"sv;
		case 0x34:
			return "Golden Nugget Casino extra buttons"sv;
		}
	}

}

namespace nesem::mappers
{
	void apply_hardware_nametable_mapping(MirroringMode mode, U16 &addr) noexcept
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

	int rom_prgrom_banks(const NesRom &rom, BankSize bank_size) noexcept
	{
		if (rom.v2)
			return int(rom.v2->prgrom.size / bank_size);

		return (rom.v1.prg_rom_size * bank_16k) / bank_size;
	}

	int rom_chrrom_banks(const NesRom &rom, BankSize bank_size) noexcept
	{
		if (rom.v2)
			return int(rom.v2->chrrom.has_value() ? (rom.v2->chrrom->size / bank_size) : 0);

		return (rom.v1.chr_rom_size * bank_8k) / bank_size;
	}

	int rom_chr_banks(const NesRom &rom, BankSize bank_size) noexcept
	{
		if (rom_has_chrram(rom))
			return int(rom_chrram_size(rom) / bank_size);

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
			return rom.v2->chrram.has_value() ? rom.v2->chrram->size : 0;

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
			size += rom.v2->prgram.has_value() ? rom.v2->prgram->size : 0;
			size += rom.v2->prgnvram.has_value() ? rom.v2->prgnvram->size : 0;
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
				if (rom.v2->pcb.submapper == 2)
					return true;

				return false;

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
			// The presense of CHR larger than 8 KiB unambiguously requires NINA-001, as BNROM has no CHR banking.
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
}
