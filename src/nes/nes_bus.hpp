#pragma once

#include <array>

#include "nes_types.hpp"

namespace nesem
{
	class NesCartridge;

	class NesBus final
	{
	public:
		U8 cpu_read(U16 addr) noexcept;
		void cpu_write(U16 addr, U8 value) noexcept;

		void load_cartridge(NesCartridge *cartridge) noexcept;

	private:
		// TODO: the real ram size is 4K
		std::array<U8, 256 * 256> ram;

		NesCartridge *cartridge = nullptr;
	};
}
