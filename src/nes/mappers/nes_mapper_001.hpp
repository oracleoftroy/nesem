#pragma once

#include <optional>
#include <vector>

#include "../nes_cartridge.hpp"
#include "nes_rom.hpp"

namespace nesem::mappers
{

	class NesMapper001 final : public NesCartridge
	{
		enum class PrgRamMode
		{
			Normal,
			SZROM,
		};

	public:
		static constexpr int ines_mapper = 1;
		explicit NesMapper001(const Nes &nes, NesRom &&rom) noexcept;

	private:
		void reset() noexcept override;
		Banks report_cpu_mapping() const noexcept override;

		U8 cpu_read(U16 addr) noexcept override;
		void cpu_write(U16 addr, U8 value) noexcept override;
		std::optional<U8> ppu_read(U16 &addr) noexcept override;
		bool ppu_write(U16 &addr, U8 value) noexcept override;

		void nt_mirroring(U16 &addr) noexcept;
		std::optional<U8> shift(U8 value) noexcept;

		struct PrgRomBanks
		{
			U8 bank;
			U8 first_bank;
			U8 last_bank;
		};

		PrgRomBanks calculate_banks() const noexcept;
		size_t map_prgram_addr(U16 addr) const noexcept;
		size_t map_prgrom_addr(U16 addr) const noexcept;
		size_t map_ppu_addr(U16 addr) const noexcept;

	private:
		PrgRamMode prg_ram_mode = PrgRamMode::Normal;
		std::vector<U8> prg_ram;

		// registers
		U8 load_counter = 0;
		U8 load_shifter = 0;
		U64 last_write_cycle = 0;

		U8 control = 0;
		U8 chr_bank_0 = 0; // bank for ppu [0000 - 1000), or entire address range [0000 - 2000) if 8k mode
		U8 chr_bank_1 = 0; // bank for ppu [1000 - 2000), or ignored in 8k mode
		U8 prg_bank = 0;

		U8 chr_bank_mask = 0;
	};
}
