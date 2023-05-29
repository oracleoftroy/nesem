#pragma once

#include <array>

#include <nes_cartridge.hpp>

#include "../nes_cartridge_loader.hpp"

#include <util/flags.hpp>

namespace nesem::mappers
{
	enum class PpuStateMirror : U8
	{
		none = 0,
		sprite8x16 = 0x01,
		show_background = 0x02,
		show_sprites = 0x4,
	};

	class NesMapper005 final : public NesCartridge
	{
		REGISTER_MAPPER(5, NesMapper005);

	public:
		explicit NesMapper005(const Nes &nes, NesRom &&rom) noexcept;

	private:
		void reset() noexcept override;
		Banks report_cpu_mapping() const noexcept override;
		Banks report_ppu_mapping() const noexcept override;
		MirroringMode mirroring() const noexcept override;

		size_t map_addr_cpu(Addr addr) const noexcept;
		size_t map_addr_ppu(Addr addr) const noexcept;

		U8 on_cpu_peek(Addr addr) const noexcept override;
		void on_cpu_write(Addr addr, U8 value) noexcept override;

		std::optional<U8> on_ppu_peek(Addr &addr) const noexcept override;
		std::optional<U8> on_ppu_read(Addr &addr) noexcept override;
		bool on_ppu_write(Addr &addr, U8 value) noexcept override;

	private:
		util::Flags<PpuStateMirror> ppu_state;
		U8 prg_mode = 0xFF;
		U8 chr_mode = 0xFF;
		U8 prg_ram_protect = 0xFF;
		U8 internal_ram_mode = 0xFF;
		U8 nametable_mapping = 0xFF;
		U8 fill_mode_tile = 0xFF;
		U8 fill_mode_color = 0xFF;
		U8 nametable_fill = 0xFF;

		std::array<U8, 5> prg_banks{0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
		std::array<U8, 12> chr_banks{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

		U8 vertical_split_mode{};
		U8 vertical_split_scroll{};
		U8 vertical_split_bank{};

		U8 scanline_irq_compare{};
		bool scanline_irq_enabled{};
		int current_scanline{};
		U8 mul_a{};
		U8 mul_b{};
		U16 mul_ans{};
	};
}
