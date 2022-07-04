#pragma once

#include "../nes_cartridge.hpp"
#include "nes_rom.hpp"

namespace nesem::mappers
{
	class NesMapper003 final : public NesCartridge
	{
	public:
		static constexpr int ines_mapper = 3;
		explicit NesMapper003(const Nes &nes, NesRom &&rom) noexcept;

	private:
		void reset() noexcept override;
		Banks report_cpu_mapping() const noexcept override;
		Banks report_ppu_mapping() const noexcept override;

		U8 on_cpu_read(U16 addr) noexcept override;
		void on_cpu_write(U16 addr, U8 value) noexcept override;
		std::optional<U8> on_ppu_read(U16 &addr) noexcept override;
		bool on_ppu_write(U16 &addr, U8 value) noexcept override;

		U8 bank_select = 0;
	};
}
