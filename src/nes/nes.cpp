#include "nes.hpp"

#include <utility>

#include "nes_cartridge.hpp"

namespace nesem
{
	Nes::Nes(DrawFn draw)
		: draw(std::move(draw)),
		  nes_bus(this),
		  nes_cpu(this),
		  nes_ppu(this),
		  nes_clock(this)
	{
	}

	bool Nes::load_rom(const std::filesystem::path &filename) noexcept
	{
		unload_rom();

		auto rom = read_rom(filename);
		if (!rom)
			return false;

		nes_cartridge = std::make_unique<NesCartridge>(std::move(rom.value()));
		nes_bus.load_cartridge(nes_cartridge.get());
		nes_ppu.load_cartridge(nes_cartridge.get());
		reset();

		return true;
	}

	void Nes::unload_rom() noexcept
	{
		nes_bus.load_cartridge(nullptr);
		nes_ppu.load_cartridge(nullptr);

		nes_cartridge = nullptr;
	}

	void Nes::reset() noexcept
	{
		nes_cpu.reset();
		nes_ppu.reset();
	}

	void Nes::tick(double deltatime)
	{
		auto dt = std::chrono::duration<double>(deltatime);
		nes_clock.tick(duration_cast<ClockRate::duration>(dt));
	}

	void Nes::screen_out(int x, int y, int color_index) noexcept
	{
		if (draw) [[likely]]
			draw(x, y, color_index);
	}

	NesBus &Nes::bus() noexcept
	{
		return nes_bus;
	}

	NesCpu &Nes::cpu() noexcept
	{
		return nes_cpu;
	}

	NesPpu &Nes::ppu() noexcept
	{
		return nes_ppu;
	}

}
