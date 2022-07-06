#pragma once

#include <array>

#include "nes_types.hpp"

namespace nesem
{
	class Nes;
	class NesCartridge;

	// Indicate whether this read or write is ready to be handled by the issuing device
	// the 6502 always issues a read or write every cycle. Cartridges will observe and
	// respond to reads/writes even if the CPU ignores them, but the PPU is a bit lazy
	// about when it actually handles the op. I presume the PPU and CPU are timed so that
	// the PPU handles the "real" operation and not the reads or writes the CPU performs
	// as it is configuring the operation
	enum class NesBusOp
	{
		pending, // the issuer is no yet ready to handle this read/write
		ready, // The issuer is ready to handle this read/write immediately
	};

	class NesBus final
	{
	public:
		NesBus(Nes *nes) noexcept;

		// just read addr without any additional handling, useful for visualizers, debuggers, etc
		U8 peek(U16 addr) const noexcept;

		U8 read(U16 addr, NesBusOp op) noexcept;
		void write(U16 addr, U8 value, NesBusOp op) noexcept;

		void load_cartridge(NesCartridge *cartridge) noexcept;

		U8 open_bus_read() const noexcept;

	private:
		Nes *nes = nullptr;
		std::array<U8, 0x800> ram;
		NesCartridge *cartridge = nullptr;

		bool poll_input = false;
		U8 controller1 = 0;
		U8 controller2 = 0;

		U8 last_read_value = 0;
	};
}
