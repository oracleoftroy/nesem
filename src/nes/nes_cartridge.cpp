#include "nes_cartridge.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <string_view>
#include <vector>

#include <util/logging.hpp>

namespace nesem
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

		if (has_inst_rom)
			LOG_WARN("ROM has INST-ROM data, but we are ignoring it");

		return std::make_optional<NesRom>(version, mapper, prg_rom_size, chr_rom_size, std::move(prg_rom), std::move(chr_rom));
	}

	NesCartridge::NesCartridge(const NesRom &rom) noexcept
		: rom(std::move(rom))
	{
		// TODO: support more mappers
		CHECK(rom.mapper == 0, "We only support mapper 0 for now");
	}

	// attempt a read of the cartridge.
	// if the cart handles it, returns byte read, else returns std::nullopt
	std::optional<U8> NesCartridge::cpu_read(U16 addr) noexcept
	{
		if (addr < 0x8000)
			return std::nullopt;

		U16 addr_mask = 0x7FFF;
		if (rom.prg_rom_size == 1)
			addr_mask = 0x3FFF;

		return rom.prg_rom[addr & addr_mask];
	}

	// attempt a write to the cartridge.
	// if the cart handles it, returns true, else returns false
	bool NesCartridge::cpu_write([[maybe_unused]] U16 addr, [[maybe_unused]] U8 value) noexcept
	{
		// TODO: support me
		return false;
	}

}
