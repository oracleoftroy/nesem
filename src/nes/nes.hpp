#pragma once

#include "nes_bus.hpp"
#include "nes_cartridge.hpp"
#include "nes_clock.hpp"
#include "nes_cpu.hpp"

namespace nesem
{
	class Nes final
	{
	public:
		Nes();

		bool load_rom(const std::filesystem::path &filename) noexcept;
		void unload_rom() noexcept;

		void tick(double deltatime);

	private:
		NesBus bus;
		NesCpu cpu;
		NesClock clock;

		std::unique_ptr<NesCartridge> cartridge;
	};
}
