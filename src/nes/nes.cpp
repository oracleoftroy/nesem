#include "nes.hpp"

#include <utility>

#include "nes_cartridge.hpp"

#include <util/logging.hpp>

namespace nesem
{
	Nes::Nes(NesSettings &&settings)
		: on_error(std::move(settings.error)),
		  draw(std::move(settings.draw)),
		  frame_ready(std::move(settings.frame_ready)),
		  player1_input(std::move(settings.player1)),
		  player2_input(std::move(settings.player2)),
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

		auto cart = load_cartridge(*this, *rom);
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

		return (nes_cartridge && nes_cartridge->irq()) || nes_apu.irq();
	}

	void Nes::screen_out(int x, int y, U8 color_index) noexcept
	{
		if (draw) [[likely]]
			draw(x, y, color_index);
	}

	void Nes::frame_complete() noexcept
	{
		if (frame_ready) [[likely]]
			frame_ready();
	}

	NesInputDevice &Nes::player1() noexcept
	{
		if (!player1_input)
			LOG_ERROR("No input device for player 1?");

		return *player1_input;
	}

	NesInputDevice &Nes::player2() noexcept
	{
		if (!player2_input)
			LOG_ERROR("No input device for player 2?");

		return *player2_input;
	}

	const NesCartridge *Nes::cartridge() const noexcept
	{
		return nes_cartridge.get();
	}
}
