#pragma once

#include "nes_bus.hpp"
#include "nes_cartridge.hpp"
#include "nes_clock.hpp"
#include "nes_cpu.hpp"
#include "nes_ppu.hpp"

namespace nesem
{
	class Nes final
	{
	public:
		Nes();

		bool load_rom(const std::filesystem::path &filename) noexcept;
		void unload_rom() noexcept;

		void tick(double deltatime);

		NesBus &bus() noexcept;
		NesCpu &cpu() noexcept;
		NesPpu &ppu() noexcept;

	private:
		NesBus nes_bus;
		NesCpu nes_cpu;
		NesPpu nes_ppu;
		NesClock nes_clock;

		std::unique_ptr<NesCartridge> nes_cartridge;
	};
}
