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
		explicit NesMapper004(const Nes &nes, NesRom &&rom) noexcept;

	private:
		void reset() noexcept override;
		Banks report_cpu_mapping() const noexcept override;
		Banks report_ppu_mapping() const noexcept override;
		MirroringMode mirroring() const noexcept;

		size_t map_addr_cpu(U16 addr) const noexcept;
		size_t map_addr_ppu(U16 addr) const noexcept;

		void update_irq(U16 addr) noexcept;

		U8 on_cpu_peek(U16 addr) const noexcept override;
		std::optional<U8> on_ppu_peek(U16 &addr) const noexcept override;

		void on_cpu_write(U16 addr, U8 value) noexcept override;
		std::optional<U8> on_ppu_read(U16 &addr) noexcept override;
		bool on_ppu_write(U16 &addr, U8 value) noexcept override;

	private:
		std::vector<U8> prg_ram;

		// registers
		U8 bank_select = 0; // 8000-9FFE even
		std::array<U8, 8> bank_map{}; // 8001-9FFF odd

		U8 mirror = 0; // A000-BFFE even
		U8 prg_ram_protect = 0; // A001-BFFF odd

		U8 irq_latch = 0; // C000-DFFE even
		bool irq_reload = false; // C001-DFFF odd
		U8 irq_counter = 0;

		// E000-FFFF, even to disable, odd to enable
		bool irq_enabled = false;

		U16 a12 = 0;
		U64 cycle_low = 0;
	};
}
