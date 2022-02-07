#include "nes.hpp"

#include <utility>

#include "nes_cartridge.hpp"

namespace nesem
{
	Nes::Nes()
		: cpu(&bus), clock(&cpu)
	{
	}

	bool Nes::load_rom(const std::filesystem::path &filename) noexcept
	{
		auto rom = read_rom(filename);
		if (!rom)
			return false;

		cartridge = std::make_unique<NesCartridge>(std::move(rom.value()));
		bus.load_cartridge(cartridge.get());
		cpu.reset(0xC000);

		return true;
	}

	void Nes::unload_rom() noexcept
	{
		cartridge = nullptr;
	}

	void Nes::tick(double deltatime)
	{
		auto dt = std::chrono::duration<double>(deltatime);
		clock.tick(duration_cast<ClockRate::duration>(dt));
	}
}
