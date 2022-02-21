#pragma once

#include <array>

#include "nes_types.hpp"

namespace nesem
{
	class Nes;
	class NesCartridge;

	class NesBus final
	{
	public:
		NesBus(Nes *nes) noexcept;

		U8 read(U16 addr) noexcept;
		void write(U16 addr, U8 value) noexcept;

		void load_cartridge(NesCartridge *cartridge) noexcept;

	private:
		Nes *nes = nullptr;
		std::array<U8, 0x7FF> ram;
		NesCartridge *cartridge = nullptr;

		bool poll_input = false;
		U8 controller1 = 0;
		U8 controller2 = 0;
	};
}
