#pragma once

#include <array>

#include "../nes_cartridge.hpp"
#include "nes_rom.hpp"

namespace nesem::mappers
{
	class NesMapper004 final : public NesCartridge
	{
	public:
		static constexpr int ines_mapper = 4;
		explicit NesMapper004(NesRom &&rom) noexcept;

	private:
		void reset() noexcept override;

		size_t map_addr_cpu(U16 addr) noexcept;
		size_t map_addr_ppu(U16 addr) noexcept;

		void update_irq(U16 addr) noexcept;

		U8 cpu_read(U16 addr) noexcept override;
		void cpu_write(U16 addr, U8 value) noexcept override;
		std::optional<U8> ppu_read(U16 &addr) noexcept override;
		bool ppu_write(U16 &addr, U8 value) noexcept override;

	private:
		std::vector<U8> prg_ram;

		// registers
		U8 bank_select = 0; // 8000-9FFE even
		std::array<U8, 8> bank_map{}; // 8001-9FFF odd

		U8 mirroring = 0; // A000-BFFE even
		U8 prg_ram_protect = 0; // A001-BFFF odd

		U8 irq_latch = 0; // C000-DFFE even
		bool irq_reload = false; // C001-DFFF odd
		U8 irq_counter = 0;

		// E000-FFFF, even to disable, odd to enable
		bool irq_enabled = false;

		U16 a12 = 0;
	};
}
