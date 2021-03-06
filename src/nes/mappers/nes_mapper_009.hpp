#pragma once

#include "../nes_cartridge.hpp"

namespace nesem::mappers
{
	class NesMapper009 final : public NesCartridge
	{
	public:
		static constexpr int ines_mapper = 9;
		explicit NesMapper009(const Nes &nes, NesRom &&rom) noexcept;

	private:
		void reset() noexcept override;

		Banks report_cpu_mapping() const noexcept override;
		Banks report_ppu_mapping() const noexcept override;

		MirroringMode mirroring() const noexcept override;

		U8 on_cpu_peek(U16 addr) const noexcept override;
		void on_cpu_write(U16 addr, U8 value) noexcept override;

		std::optional<U8> on_ppu_peek(U16 &addr) const noexcept override;
		std::optional<U8> on_ppu_read(U16 &addr) noexcept override;
		bool on_ppu_write(U16 &addr, U8 value) noexcept override;

	private:
		// registers
		U8 prgrom_bank; // $A000-$AFFF
		U8 chr_0_fd; // $B000-$BFFF
		U8 chr_0_fe; // $C000-$CFFF
		U8 chr_1_fd; // $D000-$DFFF
		U8 chr_1_fe; // $E000-$EFFF
		U8 mirror; // $F000-$FFFF

		bool chr_0; // current latch value for chr $0000-0FFF, false means use fd, true use fe
		bool chr_1; // current latch value for chr $1000-1FFF, false means use fd, true use fe
	};
}
