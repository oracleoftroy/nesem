#pragma once

#include "../nes_cartridge.hpp"

namespace nesem::mappers
{
	class NesMapper000 final : public NesCartridge
	{
		REGISTER_MAPPER(0, NesMapper000);

	public:
		explicit NesMapper000(const Nes &nes, NesRom &&rom) noexcept;

	private:
		void reset() noexcept override;
		Banks report_cpu_mapping() const noexcept override;
		Banks report_ppu_mapping() const noexcept override;

		U8 on_cpu_peek(Addr addr) const noexcept override;
		std::optional<U8> on_ppu_peek(Addr &addr) const noexcept override;

		void on_cpu_write(Addr addr, U8 value) noexcept override;
		bool on_ppu_write(Addr &addr, U8 value) noexcept override;
	};
}
