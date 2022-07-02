#include "nes_rom_loader.hpp"

#include <array>
#include <fstream>

#include <cryptopp/hex.h>
#include <cryptopp/sha.h>
#include <fmt/format.h>
#include <tinyxml2.h>

#include <util/logging.hpp>

namespace nesem
{
	namespace
	{
		using namespace mappers::ines_2;
		using namespace std::string_view_literals;

		void log_error(const tinyxml2::XMLDocument &doc)
		{
			LOG_WARN("{} line {}: {}\n", doc.ErrorName(), doc.ErrorLineNum(), doc.ErrorStr());
		}

		PrgRom read_prgrom(const tinyxml2::XMLElement *element) noexcept
		{
			if (!element || element->Name() != "prgrom"sv)
				return {};

			return {
				.size = element->Unsigned64Attribute("size"),
				.crc32 = element->Attribute("crc32"),
				.sha1 = element->Attribute("sha1"),
				.sum16 = element->Attribute("sum16"),
			};
		}

		std::optional<ChrRom> read_chrrom(const tinyxml2::XMLElement *element) noexcept
		{
			if (!element || element->Name() != "chrrom"sv)
				return {};

			return ChrRom{
				.size = element->Unsigned64Attribute("size"),
				.crc32 = element->Attribute("crc32"),
				.sha1 = element->Attribute("sha1"),
				.sum16 = element->Attribute("sum16"),
			};
		}

		Rom read_rom(const tinyxml2::XMLElement *element) noexcept
		{
			if (!element || element->Name() != "rom"sv)
				return {};

			return {
				.size = element->Unsigned64Attribute("size"),
				.crc32 = element->Attribute("crc32"),
				.sha1 = element->Attribute("sha1"),
			};
		}

		Pcb read_pcb(const tinyxml2::XMLElement *element) noexcept
		{
			if (!element || element->Name() != "pcb"sv)
				return {};

			constexpr auto mirroring = [](std::string_view m) {
				if (m == "H")
					return MirroringMode::horizontal;
				if (m == "V")
					return MirroringMode::vertical;
				if (m == "1")
					return MirroringMode::one_screen;
				if (m == "4")
					return MirroringMode::four_screen;

				LOG_CRITICAL("Unexpected mirroring mode {}", m);
				return MirroringMode::horizontal;
			};

			return {
				.mapper = element->IntAttribute("mapper"),
				.submapper = element->IntAttribute("submapper"),
				.mirroring = mirroring(element->Attribute("mirroring")),
				.battery = element->BoolAttribute("battery"),
			};
		}

		Console read_console(const tinyxml2::XMLElement *element) noexcept
		{
			if (!element || element->Name() != "console"sv)
				return {};

			return {
				.type = element->IntAttribute("type"),
				.region = element->IntAttribute("region"),
			};
		}

		Expansion read_expansion(const tinyxml2::XMLElement *element) noexcept
		{
			if (!element || element->Name() != "expansion"sv)
				return {};

			return {
				.type = element->IntAttribute("type"),
			};
		}

		std::optional<ChrRam> read_chrram(const tinyxml2::XMLElement *element) noexcept
		{
			if (!element || element->Name() != "chrram"sv)
				return {};

			return ChrRam{
				.size = element->Unsigned64Attribute("size"),
			};
		}

		std::optional<PrgNvram> read_prgnvram(const tinyxml2::XMLElement *element) noexcept
		{
			if (!element || element->Name() != "prgnvram"sv)
				return {};

			return PrgNvram{
				.size = element->Unsigned64Attribute("size"),
			};
		}

		std::optional<PrgRam> read_prgram(const tinyxml2::XMLElement *element) noexcept
		{
			if (!element || element->Name() != "prgram"sv)
				return {};

			return PrgRam{
				.size = element->Unsigned64Attribute("size"),
			};
		}

		std::optional<MiscRom> read_miscrom(const tinyxml2::XMLElement *element) noexcept
		{
			if (!element || element->Name() != "miscrom"sv)
				return {};

			return MiscRom{
				.size = element->Unsigned64Attribute("size"),
				.crc32 = element->Attribute("crc32"),
				.sha1 = element->Attribute("sha1"),
				.number = element->IntAttribute("number"),
			};
		}

		std::optional<Vs> read_vs(const tinyxml2::XMLElement *element) noexcept
		{
			if (!element || element->Name() != "vs"sv)
				return {};

			return Vs{
				.hardware = element->IntAttribute("hardware"),
				.ppu = element->IntAttribute("ppu"),
			};
		}

		std::optional<ChrNvram> read_chrnvram(const tinyxml2::XMLElement *element) noexcept
		{
			if (!element || element->Name() != "chrnvram"sv)
				return {};

			return ChrNvram{
				.size = element->Unsigned64Attribute("size"),
			};
		}

		std::optional<Trainer> read_trainer(const tinyxml2::XMLElement *element) noexcept
		{
			if (!element || element->Name() != "trainer"sv)
				return {};

			return Trainer{
				.size = element->Unsigned64Attribute("size"),
				.crc32 = std::string(element->Attribute("crc32")),
				.sha1 = std::string(element->Attribute("sha1")),
			};
		}

		std::vector<RomData> load_nes20db_xml(tinyxml2::XMLDocument &doc)
		{
			auto game = doc.FirstChildElement("nes20db")->FirstChildElement("game");
			std::vector<RomData> roms;

			while (!doc.Error() && game)
			{
				roms.emplace_back(
					read_prgrom(game->FirstChildElement("prgrom")),
					read_rom(game->FirstChildElement("rom")),
					read_pcb(game->FirstChildElement("pcb")),
					read_console(game->FirstChildElement("console")),
					read_expansion(game->FirstChildElement("expansion")),
					read_chrrom(game->FirstChildElement("chrrom")),
					read_chrram(game->FirstChildElement("chrram")),
					read_prgnvram(game->FirstChildElement("prgnvram")),
					read_prgram(game->FirstChildElement("prgram")),
					read_miscrom(game->FirstChildElement("miscrom")),
					read_vs(game->FirstChildElement("vs")),
					read_chrnvram(game->FirstChildElement("chrnvram")),
					read_trainer(game->FirstChildElement("trainer")));

				game = game->NextSiblingElement("game");
			}

			if (doc.Error())
			{
				log_error(doc);
				return {};
			}

			return roms;
		}

		std::vector<mappers::ines_2::RomData> load_nes20db_xml(const std::filesystem::path &db_file)
		{
			tinyxml2::XMLDocument doc;
			auto err = doc.LoadFile(db_file.string().c_str());

			if (err != tinyxml2::XML_SUCCESS)
			{
				log_error(doc);
				return {};
			}

			return load_nes20db_xml(doc);
		}
	}

	NesRomLoader NesRomLoader::create(const std::filesystem::path &nes20db_file)
	{
		return NesRomLoader{load_nes20db_xml(nes20db_file)};
	}

	NesRomLoader::NesRomLoader(std::vector<mappers::ines_2::RomData> &&roms)
		: roms(std::move(roms))
	{
		build_indexes();
	}

	void NesRomLoader::build_indexes()
	{
		for (size_t index = 0; index < size(roms); ++index)
			rom_sha1_to_index.emplace(roms[index].rom.sha1, index);
	}

	std::optional<mappers::NesRom> NesRomLoader::load_rom(const std::filesystem::path &filename) noexcept
	{
		using namespace std::string_view_literals;

		LOG_INFO("Loading {}", filename.string());
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

		auto mirroring = (header[6] & 0b0001) ? mappers::ines_1::Mirroring::vertical : mappers::ines_1::Mirroring::horizontal;
		bool mirror_override = (header[6] & 0b1000) != 0;

		bool has_battery = (header[6] & 0b00000010) > 0;

		// size in 16K units
		int prg_rom_size = header[4];

		// size in 8K units
		int chr_rom_size = header[5];

		if (has_trainer)
		{
			LOG_WARN("ROM has trainer data, but we are ignoring it");
			file.seekg(512, std::ios::cur);
		}

		auto result = mappers::NesRom{
			.prg_rom = std::vector<U8>(prg_rom_size * mappers::bank_16k),
			.chr_rom = std::vector<U8>(chr_rom_size * mappers::bank_8k),
			.v1 = mappers::ines_1::RomData{mapper, mirroring, mirror_override, prg_rom_size, chr_rom_size, has_battery},
		};

		if (!file.read(reinterpret_cast<char *>(data(result.prg_rom)), size(result.prg_rom)))
		{
			LOG_WARN("Error reading PRG_ROM data");
			return std::nullopt;
		}

		if (!file.read(reinterpret_cast<char *>(data(result.chr_rom)), size(result.chr_rom)))
		{
			LOG_WARN("Error reading CHR_ROM data");
			return std::nullopt;
		}

		if (has_inst_rom)
			LOG_WARN("ROM has INST-ROM data, but we are ignoring it");

		result.v2 = find_rom_data(result.prg_rom, result.chr_rom);

		return result;
	}

	std::optional<mappers::ines_2::RomData> NesRomLoader::find_rom_data(const std::vector<U8> &prgrom, const std::vector<U8> &chrrom)
	{
		auto rom_sha1 = [&] {
			std::array<CryptoPP::byte, CryptoPP::SHA1::DIGESTSIZE> digest;

			CryptoPP::SHA1 sha;
			sha.Update(data(prgrom), size(prgrom));
			sha.Update(data(chrrom), size(chrrom));
			sha.Final(data(digest));

			std::string output;

			auto encoder = CryptoPP::HexEncoder(new CryptoPP::StringSink(output));
			encoder.Put(data(digest), size(digest));
			encoder.MessageEnd();

			return output;
		}();

		if (auto it = rom_sha1_to_index.find(rom_sha1); it != end(rom_sha1_to_index))
			return roms[it->second];

		LOG_WARN("ROM not found in DB");
		return {};
	}
}
