#pragma once

#include <array>

#include "../nes_cartridge.hpp"

namespace nesem::mappers
{
	// consider adding a fallback that acts like MMC3C without ram protect?
	enum class NesMapper004Variants
	{
		MMC3C = 0, // default, maps to submapper 0
		MMC6 = 1, // uses regs $8000 and $A001 slightly differently, 1K prgram at $7000-7FFF, maps to submapper 1
		MC_ACC = 3, // IRQ on falling edge of A12, maps to submapper 3
		MMC3A = 4, // IRQ disabled if latch is 0, maps to submapper 4
	};

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
		U8 do_read_ram(size_t addr) const noexcept;
		bool do_read_write(size_t addr, U8 value) noexcept;

		NesMapper004Variants variant = NesMapper004Variants::MMC3C;

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
