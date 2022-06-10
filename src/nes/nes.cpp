#include "nes.hpp"

#include <utility>

#include "nes_cartridge.hpp"

#include <util/logging.hpp>

namespace nesem
{
	Nes::Nes(NesSettings &&settings)
		: on_error(std::move(settings.error)),
		  draw(std::move(settings.draw)),
		  player1(std::move(settings.player1)),
		  player2(std::move(settings.player2)),
		  nes_bus(this),
		  nes_cpu(this),
		  nes_ppu(this),
		  nes_apu(this),
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

	void Nes::error(std::string_view message) noexcept
	{
		nes_clock.stop();

		if (on_error)
			on_error(message);
		else
		{
			LOG_CRITICAL("Error encountered, but no error handler attached, resetting...");
			DEBUG_BREAK(); // give us a chance to debug before resetting
			reset();
		}
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

	bool Nes::interrupt_requested() noexcept
	{
		// components requesting an interrupt should hold the interrupt signal until the CPU writes back saying
		// it handled the interrupt. Multiple devices can signal an irq at the same time, so this gives us a
		// central location where all active interrupt requests can be accessed at once. Right now, only the APU
		// signals irqs, but some mappers can request interrupts as well as I understand. It is the programmer's
		// responsibility to figure out which device(s) signaled the interrupt and handle it appropriately.

		return nes_apu.irq();
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

	NesApu &Nes::apu() noexcept
	{
		return nes_apu;
	}
}
