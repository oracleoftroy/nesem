#pragma once

#include <filesystem>
#include <optional>

#include "mappers/nes_rom.hpp"
#include "nes_types.hpp"

namespace nesem
{
	class NesCartridge
	{
	public:
		explicit NesCartridge(mappers::NesRom &&rom) noexcept;

		virtual ~NesCartridge() = default;

		virtual void reset() noexcept = 0;

		virtual U8 cpu_read(U16 addr) noexcept = 0;
		virtual void cpu_write(U16 addr, U8 value) noexcept = 0;

		virtual std::optional<U8> ppu_read(U16 &addr) noexcept = 0;
		virtual bool ppu_write(U16 &addr, U8 value) noexcept = 0;

		const mappers::NesRom &get_rom() noexcept;
		bool irq() noexcept;

	protected:
		mappers::NesRom rom;
		bool irq_signaled = false;
	};

	std::unique_ptr<NesCartridge> load_cartridge(mappers::NesRom rom) noexcept;
}
