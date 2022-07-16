#pragma once

#include "../nes_cartridge.hpp"

namespace nesem::mappers
{
	class NesMapper066 final : public NesCartridge
	{
	public:
		static constexpr int ines_mapper = 66;
		explicit NesMapper066(const Nes &nes, NesRom &&rom) noexcept;

	private:
		void reset() noexcept override;
		Banks report_cpu_mapping() const noexcept override;
		Banks report_ppu_mapping() const noexcept override;

		U8 on_cpu_peek(U16 addr) const noexcept override;
		std::optional<U8> on_ppu_peek(U16 &addr) const noexcept override;

		void on_cpu_write(U16 addr, U8 value) noexcept override;
		bool on_ppu_write(U16 &addr, U8 value) noexcept override;

		U8 prg_bank_select = 0;
		U8 chr_bank_select = 0;
	};
}
