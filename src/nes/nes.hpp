#pragma once

#include "nes_bus.hpp"
#include "nes_cartridge.hpp"
#include "nes_clock.hpp"
#include "nes_cpu.hpp"
#include "nes_ppu.hpp"
#include "nes_rom_loader.hpp"

namespace nesem
{
	struct NesSettings
	{
		ErrorFn error;
		DrawFn draw;
		PollInputFn player1;
		PollInputFn player2 = {};
		std::filesystem::path nes20db_filename = {};
	};

	class Nes final
	{
	public:
		Nes(const NesSettings &settings);

		bool load_rom(const std::filesystem::path &filename) noexcept;
		void unload_rom() noexcept;
		void reset() noexcept;
		void error(std::string_view message) noexcept;

		// run the system at full speed for a timeslice
		void tick(double deltatime) noexcept;

		// step the system, returns the total system time the operations took
		double step(NesClockStep step) noexcept;

		void screen_out(int x, int y, int color_index) noexcept;
		U8 poll_player1() noexcept;
		U8 poll_player2() noexcept;

		NesBus &bus() noexcept;
		NesCpu &cpu() noexcept;
		NesPpu &ppu() noexcept;

	private:
		ErrorFn on_error;
		DrawFn draw;
		PollInputFn player1;
		PollInputFn player2;

		NesBus nes_bus;
		NesCpu nes_cpu;
		NesPpu nes_ppu;
		NesClock nes_clock;
		NesRomLoader rom_loader;

		std::unique_ptr<NesCartridge> nes_cartridge;
	};
}
