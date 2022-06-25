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
		explicit NesMapper001(NesRom &&rom) noexcept;

	private:
		void reset() noexcept override;

		U8 cpu_read(U16 addr) noexcept override;
		void cpu_write(U16 addr, U8 value) noexcept override;
		std::optional<U8> ppu_read(U16 &addr) noexcept override;
		bool ppu_write(U16 &addr, U8 value) noexcept override;

		void nt_mirroring(U16 &addr) noexcept;
		void load_complete(U16 addr) noexcept;
		void update_state() noexcept;

		std::vector<U8> prg_ram;

		U8 load_counter;
		U8 load_shifter;

		struct Registers
		{
			U8 control;
			U8 chr_0;
			U8 chr_1;
			U8 prg;
			U8 chr_last; // copy of the last written chr_0 or chr_1 value for some cart specific bank switching rules
		} reg;

		enum class Prg
		{
			size_16k,
			size_32k,
		};
		Prg prg_mode;

		enum class Chr
		{
			size_4k,
			size_8k,
		};
		Chr chr_mode;

		U8 chr_bank_0;
		U8 chr_bank_1;
		U8 prg_bank_0;
		U8 prg_bank_1;

		bool prg_ram_enabled;
	};
}
