#pragma once

#include <nes_cartridge.hpp>

#include "../nes_cartridge_loader.hpp"

namespace nesem::mappers
{
	class NesMapper002 final : public NesCartridge
	{
		REGISTER_MAPPER(2, NesMapper002);

	public:
		explicit NesMapper002(const Nes &nes, NesRom &&rom) noexcept;

	private:
		void reset() noexcept override;
		Banks report_cpu_mapping() const noexcept override;
		Banks report_ppu_mapping() const noexcept override;

		U8 on_cpu_peek(Addr addr) const noexcept override;
		std::optional<U8> on_ppu_peek(Addr &addr) const noexcept override;

		void on_cpu_write(Addr addr, U8 value) noexcept override;
		bool on_ppu_write(Addr &addr, U8 value) noexcept override;

		U8 bank_select = 0;
	};
}
