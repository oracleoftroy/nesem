#pragma once

#include <array>

#include <nes_types.hpp>

namespace nesem
{
	class Nes;
	class NesCartridge;

	class NesBus final
	{
	public:
		NesBus(Nes *nes) noexcept;

		void clock() noexcept;

		// just read addr without any additional handling, useful for visualizers, debuggers, etc
		U8 peek(Addr addr) const noexcept;

		U8 read(Addr addr, NesBusOp op) noexcept;
		void write(Addr addr, U8 value, NesBusOp op) noexcept;

		void load_cartridge(NesCartridge *cartridge) noexcept;

		U8 open_bus_read() const noexcept;

	private:
		Nes *nes = nullptr;
		std::array<U8, 0x800> ram;
		NesCartridge *cartridge = nullptr;

		bool poll_input = false;
		U8 controller1 = 0;
		U8 controller2 = 0;

		U8 last_read_value = 0;
	};
}
