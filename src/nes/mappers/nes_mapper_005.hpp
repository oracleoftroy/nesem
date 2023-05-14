#pragma once

#include <array>

#include "../nes_cartridge.hpp"

#include <util/enum.hpp>

namespace nesem::mappers
{
	enum class PpuStateMirror : U8
	{
		none = 0,
		sprite8x16 = 0x01,
		show_background = 0x02,
		show_sprites = 0x4,
	};

	MAKE_FLAGS_ENUM(PpuStateMirror);

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

		size_t map_addr_cpu(U16 addr) const noexcept;
		size_t map_addr_ppu(U16 addr) const noexcept;

		U8 on_cpu_peek(U16 addr) const noexcept override;
		void on_cpu_write(U16 addr, U8 value) noexcept override;

		std::optional<U8> on_ppu_peek(U16 &addr) const noexcept override;
		std::optional<U8> on_ppu_read(U16 &addr) noexcept override;
		bool on_ppu_write(U16 &addr, U8 value) noexcept override;

	private:
		PpuStateMirror ppu_state;
		U8 prg_mode;
		U8 chr_mode;
		U8 prg_ram_protect;
		U8 internal_ram_mode;
		U8 nametable_mapping;
		U8 fill_mode_tile;
		U8 fill_mode_color;
		U8 nametable_fill;

		std::array<U8, 5> prg_banks;
		std::array<U8, 12> chr_banks;

		U8 vertical_split_mode;
		U8 vertical_split_scroll;
		U8 vertical_split_bank;

		U8 scanline_irq_compare;
		bool scanline_irq_enabled;
		int current_scanline;
		U8 mul_a;
		U8 mul_b;
		U16 mul_ans;
	};
}
