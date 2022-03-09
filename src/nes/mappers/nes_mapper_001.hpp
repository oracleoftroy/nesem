#pragma once

#include <vector>

#include "../nes_cartridge.hpp"
#include "nes_rom.hpp"

namespace nesem::mappers
{
	class NesMapper001 final : public NesCartridge
	{
	public:
		static constexpr int ines_mapper = 1;
		NesMapper001(NesRom &&rom) noexcept;

	private:
		void reset() noexcept override;

		U8 cpu_read(U16 addr) noexcept override;
		void cpu_write(U16 addr, U8 value) noexcept override;
		std::optional<U8> ppu_read(U16 &addr) noexcept override;
		bool ppu_write(U16 &addr, U8 value) noexcept override;

		void nt_mirroring(U16 &addr) noexcept;
		void load_complete(U16 addr) noexcept;

		NesRom rom;
		std::vector<U8> prg_ram;

		U8 load_counter;
		U8 load_shifter;

		struct Registers
		{
			U8 control;
			U8 chr_0;
			U8 chr_1;
			U8 prg;
		} reg;

		U8 chr_bank_0;
		U8 chr_bank_1;
		U8 prg_bank_0;
		U8 prg_bank_1;

		bool prg_ram_enabled;
	};
}
