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
		Nes(DrawFn draw, PollInputFn player1, PollInputFn player2 = {});

		bool load_rom(const std::filesystem::path &filename) noexcept;
		void unload_rom() noexcept;
		void reset() noexcept;

		void tick(double deltatime);

		void screen_out(int x, int y, int color_index) noexcept;
		U8 poll_player1() noexcept;
		U8 poll_player2() noexcept;

		NesBus &bus() noexcept;
		NesCpu &cpu() noexcept;
		NesPpu &ppu() noexcept;

	private:
		DrawFn draw;
		PollInputFn player1;
		PollInputFn player2;

		NesBus nes_bus;
		NesCpu nes_cpu;
		NesPpu nes_ppu;
		NesClock nes_clock;

		std::unique_ptr<NesCartridge> nes_cartridge;
	};
}
