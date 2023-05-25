#pragma once

#include <filesystem>
#include <memory>
#include <string_view>

#include <nes_apu.hpp>
#include <nes_bus.hpp>
#include <nes_clock.hpp>
#include <nes_cpu.hpp>
#include <nes_input_device.hpp>
#include <nes_ppu.hpp>
#include <nes_rom_loader.hpp>

namespace nesem
{
	struct NesSettings
	{
		ErrorFn error;
		DrawFn draw;
		FrameReadyFn frame_ready;
		std::unique_ptr<NesInputDevice> player1 = make_null_input();
		std::unique_ptr<NesInputDevice> player2 = make_null_input();
		std::filesystem::path nes20db_filename = {};
		std::filesystem::path user_data_dir = {};
	};

	class NesCartridge;
	class NesNvram;

	class Nes final
	{
	public:
		explicit Nes(NesSettings &&settings);
		~Nes();

		bool load_rom(const std::filesystem::path &filename) noexcept;
		void unload_rom() noexcept;
		void reset() noexcept;
		void error(std::string_view message) noexcept;

		// run the system at full speed for a timeslice
		void tick(double deltatime) noexcept;

		// step the system, returns the total system time the operations took
		double step(NesClockStep step) noexcept;

		// returns true if a component is signaling an interrupt
		bool interrupt_requested() noexcept;

		void screen_out(int x, int y, U8 color_index, util::Flags<NesColorEmphasis> emphasis) noexcept;
		void frame_complete() noexcept;

		NesInputDevice &player1() noexcept;
		NesInputDevice &player2() noexcept;

		const NesCartridge *cartridge() const noexcept;

		NesNvram open_prgnvram(std::string_view rom, size_t size) const noexcept;

#if defined(__cpp_explicit_this_parameter)
		auto &bus(this auto &self) noexcept
		{
			return self.nes_bus;
		}

		auto &cpu(this auto &self) noexcept
		{
			return self.nes_cpu;
		}

		auto &ppu(this auto &self) noexcept
		{
			return self.nes_ppu;
		}

		auto &apu(this auto &self) noexcept
		{
			return self.nes_apu;
		}
#else
		auto &bus() noexcept
		{
			return nes_bus;
		}

		const auto &bus() const noexcept
		{
			return nes_bus;
		}

		auto &cpu() noexcept
		{
			return nes_cpu;
		}

		const auto &cpu() const noexcept
		{
			return nes_cpu;
		}

		auto &ppu() noexcept
		{
			return nes_ppu;
		}

		const auto &ppu() const noexcept
		{
			return nes_ppu;
		}

		auto &apu() noexcept
		{
			return nes_apu;
		}

		const auto &apu() const noexcept
		{
			return nes_apu;
		}
#endif

	private:
		ErrorFn on_error;
		DrawFn draw;
		FrameReadyFn frame_ready;
		std::unique_ptr<NesInputDevice> player1_input;
		std::unique_ptr<NesInputDevice> player2_input;

		NesBus nes_bus;
		NesCpu nes_cpu;
		NesPpu nes_ppu;
		NesApu nes_apu;
		NesClock nes_clock;
		NesRomLoader rom_loader;

		std::unique_ptr<NesCartridge> nes_cartridge;
		std::filesystem::path user_data_dir;
	};
}
