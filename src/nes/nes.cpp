#include "nes.hpp"

#include <utility>

#include "nes_cartridge.hpp"

namespace nesem
{
	Nes::Nes(const NesSettings &settings)
		: draw(std::move(settings.draw)),
		  player1(std::move(settings.player1)),
		  player2(std::move(settings.player2)),
		  nes_bus(this),
		  nes_cpu(this),
		  nes_ppu(this),
		  nes_clock(this),
		  rom_loader(NesRomLoader::create(settings.nes20db_filename))
	{
	}

	bool Nes::load_rom(const std::filesystem::path &filename) noexcept
	{
		auto rom = rom_loader.load_rom(filename);
		if (!rom)
			return false;

		auto cart = load_cartridge(*rom);
		if (!cart)
			return false;

		unload_rom();

		nes_cartridge = std::move(cart);
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
		nes_cartridge->reset();
		nes_cpu.reset();
		nes_ppu.reset();
	}

	void Nes::tick(double deltatime) noexcept
	{
		auto dt = std::chrono::duration<double>(deltatime);
		nes_clock.tick(duration_cast<ClockRate::duration>(dt));
	}

	double Nes::step(NesClockStep step) noexcept
	{
		std::chrono::duration<double> dt = nes_clock.step(step);
		return dt.count();
	}

	void Nes::screen_out(int x, int y, int color_index) noexcept
	{
		if (draw) [[likely]]
			draw(x, y, color_index);
	}

	U8 Nes::poll_player1() noexcept
	{
		if (player1)
			return static_cast<U8>(player1());

		return 0;
	}

	U8 Nes::poll_player2() noexcept
	{
		if (player2)
			return static_cast<U8>(player2());

		return 0;
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
