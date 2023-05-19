#pragma once

#include <array>
#include <bit>
#include <filesystem>
#include <optional>
#include <vector>

#include "nes_cartridge_loader.hpp"
#include "nes_nvram.hpp"
#include "nes_rom.hpp"
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

#if defined(__cpp_explicit_this_parameter)
		auto begin(this auto &&self) noexcept
		{
			return std::begin(self.banks);
		}

		auto end(this auto &&self) noexcept
		{
			return std::begin(self.banks) + self.size;
		}
#else
		auto begin() const noexcept
		{
			return std::begin(banks);
		}

		auto end() const noexcept
		{
			return std::begin(banks) + size;
		}

#endif
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

		U8 cpu_peek(Addr addr) const noexcept;
		std::optional<U8> ppu_peek(Addr &addr) const noexcept;

		U8 cpu_read(Addr addr) noexcept;
		void cpu_write(Addr addr, U8 value) noexcept;

		std::optional<U8> ppu_read(Addr &addr) noexcept;
		bool ppu_write(Addr &addr, U8 value) noexcept;

		const mappers::NesRom &rom() const noexcept;
		bool irq() noexcept;

		size_t chr_size() const noexcept;

		virtual void signal_m2(bool rising) noexcept;

	private:
		virtual U8 on_cpu_peek(Addr addr) const noexcept = 0;
		virtual U8 on_cpu_read(Addr addr) noexcept;
		virtual void on_cpu_write(Addr addr, U8 value) noexcept = 0;

		virtual std::optional<U8> on_ppu_peek(Addr &addr) const noexcept = 0;
		virtual std::optional<U8> on_ppu_read(Addr &addr) noexcept;
		virtual bool on_ppu_write(Addr &addr, U8 value) noexcept = 0;

	protected:
		U8 chr_read(size_t addr) const noexcept;
		bool chr_write(size_t addr, U8 value) noexcept;

		void signal_irq(bool signal) noexcept;

		void enable_bus_conflicts(bool enable) noexcept;
		U8 open_bus_read() const noexcept;

		// simple use case for handling prg_ram, where it is all battery backed or all volatile
		// mappers that allow both ought to use the more specific functions
		// having both is pretty rare though, nes20db only lists 48 carts total across mappers 1, 5, 164, 195, 198, 199, and 547
		size_t cpu_ram_size() const noexcept;
		U8 cpu_ram_read(size_t addr) const noexcept;
		bool cpu_ram_write(size_t addr, U8 value) noexcept;

		size_t prgram_size() const noexcept;
		U8 prgram_read(size_t addr) const noexcept;
		bool prgram_write(size_t addr, U8 value) noexcept;

		size_t prgnvram_size() const noexcept;
		U8 prgnvram_read(size_t addr) const noexcept;
		bool prgnvram_write(size_t addr, U8 value) noexcept;

	protected:
		const Nes *nes = nullptr;

	private:
		mappers::NesRom nes_rom;

		std::vector<U8> chr_ram;
		bool irq_signaled = false;
		bool emulate_bus_conflicts = false;

		std::vector<U8> prg_ram;
		NesNvram prg_nvram;
	};
}
