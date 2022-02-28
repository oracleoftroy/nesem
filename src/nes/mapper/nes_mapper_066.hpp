#pragma once

#include "../nes_cartridge.hpp"
#include "nes_rom.hpp"

namespace nesem::mapper
{
	class NesMapper066 final : public NesCartridge
	{
	public:
		static constexpr int ines_mapper = 66;
		NesMapper066(NesRom &&rom) noexcept;

	private:
		void reset() noexcept override;

		U8 cpu_read(U16 addr) noexcept override;
		void cpu_write(U16 addr, U8 value) noexcept override;
		std::optional<U8> ppu_read(U16 &addr) noexcept override;
		bool ppu_write(U16 &addr, U8 value) noexcept override;

		NesRom rom;
		U8 prg_bank_select = 0;
		U8 chr_bank_select = 0;
	};
}
