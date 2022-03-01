#include "nes_rom.hpp"

#include <array>
#include <fstream>
#include <string_view>
#include <utility>

#include <util/logging.hpp>

namespace nesem::mapper
{
	std::optional<NesRom> read_rom(const std::filesystem::path &filename) noexcept
	{
		using namespace std::string_view_literals;

		auto file = std::ifstream(filename, std::ios::binary);

		if (!file)
		{
			LOG_WARN("Could not open file '{}'", filename.string());
			return std::nullopt;
		}

		std::array<uint8_t, 16> header;

		if (!file.read(reinterpret_cast<char *>(data(header)), size(header)))
		{
			LOG_WARN("Error reading iNES Rom header, file: '{}'", filename.string());
			return std::nullopt;
		}

		// iNES rom files start with "NES" followed by EOF (0x1A)
		if (std::string_view(reinterpret_cast<char *>(data(header)), 4) != "NES\x1A"sv)
		{
			LOG_WARN("Invalid iNES Rom, file: '{}'", filename.string());
			return std::nullopt;
		}

		int version = 1;

		// check for version 2. Version 2 has bit 2 clear and bit 3 set
		if ((header[7] & 0b00001100) == 0b00001000)
			version = 2;

		// optional trainer, 512 bytes if present
		bool has_trainer = (header[6] & 0b00000100) > 0;

		// optional INST-ROM, 8192 bytes if present
		bool has_inst_rom = (header[7] & 0b00000010) > 0;

		int mapper = (header[7] & 0xF0) | (header[6] >> 4);

		NesMirroring mirroring = (header[6] & 0b0001) ? NesMirroring::vertical : NesMirroring::horizontal;
		bool mirror_override = (header[6] & 0b1000) != 0;

		// size in 16K units
		int prg_rom_size = header[4];

		// size in 8K units
		int chr_rom_size = header[5];

		if (has_trainer)
		{
			LOG_WARN("ROM has trainer data, but we are ignoring it");
			file.seekg(512, std::ios::cur);
		}

		std::vector<U8> prg_rom(prg_rom_size * 16384);
		if (!file.read(reinterpret_cast<char *>(data(prg_rom)), size(prg_rom)))
		{
			LOG_WARN("Error reading PRG_ROM data");
			return std::nullopt;
		}

		std::vector<U8> chr_rom(chr_rom_size * 8192);
		if (!file.read(reinterpret_cast<char *>(data(chr_rom)), size(chr_rom)))
		{
			LOG_WARN("Error reading CHR_ROM data");
			return std::nullopt;
		}

		// 0 indicates CHR-RAM. As there is nothing to read in that case, we previously allocated 0 bytes, but now allocate an 8K buffer
		// TODO: should this be per mapper? Or is 8K the standard size for CHR-RAM?
		if (chr_rom_size == 0)
			chr_rom.resize(8192);

		if (has_inst_rom)
			LOG_WARN("ROM has INST-ROM data, but we are ignoring it");

		return std::make_optional<NesRom>(version, mapper, mirroring, mirror_override, prg_rom_size, chr_rom_size, std::move(prg_rom), std::move(chr_rom));
	}

	void apply_hardware_nametable_mapping(const NesRom &rom, U16 &addr) noexcept
	{
		if (rom.mirroring == NesMirroring::horizontal)
		{
			// exchange the nt_x and nt_y bits
			// we assume vertical mirroring by default, so this flips the 2400-27ff
			// range with the 2800-2BFF range to achieve a horizontal mirror
			addr = (addr & ~0b0'000'11'00000'00000) |
				((addr & 0b0'000'10'00000'00000) >> 1) |
				((addr & 0b0'000'01'00000'00000) << 1);
		}
	}

}
