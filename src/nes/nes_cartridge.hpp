#pragma once

#include <array>
#include <filesystem>
#include <optional>
#include <vector>

#include "mappers/nes_rom.hpp"
#include "nes_types.hpp"

namespace nesem
{
	class Nes;

	struct Bank
	{
		U16 addr; // starting address in chr / prg memory
		U16 bank; // the bank number in
		U32 size;
	};

	struct Banks
	{
		// using a fixed size array to hold the bank configuration to avoid dynamic memory allocation
		// RVO should completely eliminate any copying. Bank is 8 bytes, and 8 * 8 + 8 == 72 bytes
		//  on the stack doesn't seem too much for a typical desktop
		static constexpr size_t N = 8;
		size_t size;
		std::array<Bank, N> banks;

		auto &&begin(this auto &&self) noexcept
		{
			return std::begin(self.banks);
		}

		auto &&end(this auto &&self) noexcept
		{
			return std::begin(self.banks) + self.size;
		}
	};

	class NesCartridge
	{
	public:
		explicit NesCartridge(const Nes &nes, mappers::NesRom &&rom) noexcept;

		virtual ~NesCartridge() = default;

		virtual void reset() noexcept = 0;

		virtual Banks report_cpu_mapping() const noexcept = 0;
		virtual Banks report_ppu_mapping() const noexcept = 0;
		virtual mappers::MirroringMode mirroring() const noexcept;

		U8 cpu_read(U16 addr) noexcept;
		void cpu_write(U16 addr, U8 value) noexcept;

		std::optional<U8> ppu_read(U16 &addr) noexcept;
		bool ppu_write(U16 &addr, U8 value) noexcept;

		const mappers::NesRom &rom() const noexcept;
		bool irq() noexcept;

		size_t chr_size() const noexcept;

	private:
		virtual U8 on_cpu_read(U16 addr) noexcept = 0;
		virtual void on_cpu_write(U16 addr, U8 value) noexcept = 0;

		virtual std::optional<U8> on_ppu_read(U16 &addr) noexcept = 0;
		virtual bool on_ppu_write(U16 &addr, U8 value) noexcept = 0;

	protected:
		U8 chr_read(size_t addr) const noexcept;
		bool chr_write(size_t addr, U8 value) noexcept;

		void signal_irq(bool signal) noexcept;

	protected:
		const Nes *nes = nullptr;

	private:
		mappers::NesRom nes_rom;

		std::vector<U8> chr_ram;
		bool irq_signaled = false;
	};

	std::unique_ptr<NesCartridge> load_cartridge(const Nes &nes, mappers::NesRom rom) noexcept;
}
