#include "nes_rom_loader.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <utility>

#include <fmt/format.h>
#include <mio/mmap.hpp>
#include <tinyxml2.h>

#include "nes_sha1.hpp"

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

		template <std::integral T>
		T read_int_attribute(const tinyxml2::XMLElement *element, const char *name)
		{
			if constexpr (std::same_as<int32_t, T>)
				return element->IntAttribute(name);
			if constexpr (std::same_as<int64_t, T>)
				return element->Int64Attribute(name);
			if constexpr (std::same_as<uint32_t, T>)
				return element->UnsignedAttribute(name);
			if constexpr (std::same_as<uint64_t, T>)
				return element->Unsigned64Attribute(name);

			std::unreachable();
		}

		PrgRom read_prgrom(const tinyxml2::XMLElement *element) noexcept
		{
			if (!element || element->Name() != "prgrom"sv)
				return {};

			return {
				.size = read_int_attribute<size_t>(element, "size"),
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
				.size = read_int_attribute<size_t>(element, "size"),
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
				.size = read_int_attribute<size_t>(element, "size"),
				.crc32 = element->Attribute("crc32"),
				.sha1 = element->Attribute("sha1"),
			};
		}

		Pcb read_pcb(const tinyxml2::XMLElement *element) noexcept
		{
			if (!element || element->Name() != "pcb"sv)
				return {};

			constexpr auto mirroring = [](std::string_view m) {
				using enum mappers::MirroringMode;
				if (m == "H")
					return horizontal;
				if (m == "V")
					return vertical;
				if (m == "1")
					return one_screen;
				if (m == "4")
					return four_screen;

				LOG_CRITICAL("Unexpected mirroring mode {}", m);
				return horizontal;
			};

			return {
				.mapper = read_int_attribute<int>(element, "mapper"),
				.submapper = read_int_attribute<int>(element, "submapper"),
				.mirroring = mirroring(element->Attribute("mirroring")),
				.battery = element->BoolAttribute("battery"),
			};
		}

		Console read_console(const tinyxml2::XMLElement *element) noexcept
		{
			if (!element || element->Name() != "console"sv)
				return {};

			return {
				.type = read_int_attribute<int>(element, "type"),
				.region = read_int_attribute<int>(element, "region"),
			};
		}

		Expansion read_expansion(const tinyxml2::XMLElement *element) noexcept
		{
			if (!element || element->Name() != "expansion"sv)
				return {};

			return Expansion{read_int_attribute<int>(element, "type")};
		}

		std::optional<ChrRam> read_chrram(const tinyxml2::XMLElement *element) noexcept
		{
			if (!element || element->Name() != "chrram"sv)
				return {};

			return ChrRam{read_int_attribute<size_t>(element, "size")};
		}

		std::optional<PrgNvram> read_prgnvram(const tinyxml2::XMLElement *element) noexcept
		{
			if (!element || element->Name() != "prgnvram"sv)
				return {};

			return PrgNvram{read_int_attribute<size_t>(element, "size")};
		}

		std::optional<PrgRam> read_prgram(const tinyxml2::XMLElement *element) noexcept
		{
			if (!element || element->Name() != "prgram"sv)
				return {};

			return PrgRam{read_int_attribute<size_t>(element, "size")};
		}

		std::optional<MiscRom> read_miscrom(const tinyxml2::XMLElement *element) noexcept
		{
			if (!element || element->Name() != "miscrom"sv)
				return {};

			return MiscRom{
				.size = read_int_attribute<size_t>(element, "size"),
				.crc32 = element->Attribute("crc32"),
				.sha1 = element->Attribute("sha1"),
				.number = read_int_attribute<int>(element, "number"),
			};
		}

		std::optional<Vs> read_vs(const tinyxml2::XMLElement *element) noexcept
		{
			if (!element || element->Name() != "vs"sv)
				return {};

			return Vs{
				.hardware = read_int_attribute<int>(element, "hardware"),
				.ppu = read_int_attribute<int>(element, "ppu"),
			};
		}

		std::optional<ChrNvram> read_chrnvram(const tinyxml2::XMLElement *element) noexcept
		{
			if (!element || element->Name() != "chrnvram"sv)
				return {};

			return ChrNvram{read_int_attribute<size_t>(element, "size")};
		}

		std::optional<Trainer> read_trainer(const tinyxml2::XMLElement *element) noexcept
		{
			if (!element || element->Name() != "trainer"sv)
				return {};

			return Trainer{
				.size = read_int_attribute<size_t>(element, "size"),
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

		mappers::ines_1::RomData read_ines_1_data(std::span<const U8> header)
		{
			int version = 1;

			// check for version 2. Version 2 has bit 2 clear and bit 3 set
			if ((header[7] & 0b00001100) == 0b00001000)
				version = 2;

			// optional trainer, 512 bytes if present
			bool has_trainer = (header[6] & 0b00000100) > 0;

			// optional INST-ROM, 8192 bytes if present
			bool has_inst_rom = (header[7] & 0b00000010) > 0;

			int mapper = (header[7] & 0xF0) | (header[6] >> 4);

			auto mirroring = mappers::MirroringMode((header[6] & 0b0001) | ((header[6] & 0b1000) >> 2));

			bool has_battery = (header[6] & 0b00000010) > 0;

			// size in 16K units
			U8 prg_rom_size = header[4];

			// size in 8K units
			U8 chr_rom_size = header[5];

			// size in 8K units
			U8 prg_ram_size = header[8];

			return mappers::ines_1::RomData{version, mapper, mirroring, prg_rom_size, chr_rom_size, prg_ram_size, has_battery, has_trainer, has_inst_rom};
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

		mio::ummap_source file;
		std::error_code ec;
		file.map(filename.string(), ec);

		if (ec)
		{
			LOG_WARN("Could not open file '{}', reason: {}", filename.string(), ec.message());
			return std::nullopt;
		}

		// bail early if we don't even have enough space for the ines header
		if (file.size() < 16)
		{
			LOG_WARN("File '{}' too small");
			return std::nullopt;
		}

		auto file_data = std::span(file.data(), file.size());

		// iNES rom files start with "NES" followed by the DOS "EOF" character (0x1A, more correctly, the ASCII/ANSI SUB character)
		constexpr auto magic = std::array<U8, 4>{'N', 'E', 'S', '\x1A'};

		// if (!std::ranges::equal(file_data.subspan(0, 4), magic))
		auto start_span = file_data.subspan(0, 4);
		if (!std::equal(begin(start_span), end(start_span), begin(magic)))
		{
			LOG_WARN("Invalid iNES Rom, file: '{}'", filename.string());
			return std::nullopt;
		}

		auto ines_1 = read_ines_1_data(file_data.subspan(0, 16));
		auto sha1 = util::sha1(file_data.subspan(16));
		auto ines_2 = find_rom_data(sha1);

		LOG_INFO("ROM file iNES version: {}", ines_1.version);

		size_t prg_rom_size = ines_2 ? ines_2->prgrom.size : ines_1.prg_rom_size * bank_16k;
		size_t chr_rom_size = ines_2 && ines_2->chrrom ? ines_2->chrrom->size : ines_1.chr_rom_size * bank_8k;
		size_t trainer_size = ines_2 && ines_2->trainer ? ines_2->trainer->size : ines_1.has_trainer * 512;

		size_t expected_rom_size = 16 + prg_rom_size + chr_rom_size + trainer_size;

		if (expected_rom_size != file.size())
			LOG_WARN("ROM reports size {:L} but size is {:L}", expected_rom_size, file.size());

		auto prg_rom_start = file.begin() + 16 + trainer_size;
		auto chr_rom_start = prg_rom_start + prg_rom_size;

		auto result = mappers::NesRom{
			.prg_rom = std::vector<U8>(prg_rom_start, prg_rom_start + prg_rom_size),
			.chr_rom = std::vector<U8>(chr_rom_start, chr_rom_start + chr_rom_size),
			.sha1 = sha1,
			.v1 = ines_1,
			.v2 = ines_2,
		};

		if (trainer_size > 0)
			LOG_WARN("ROM has INST-ROM data, but we are ignoring it");

		return result;
	}

	std::optional<mappers::ines_2::RomData> NesRomLoader::find_rom_data(std::string_view sha1)
	{
		if (auto it = rom_sha1_to_index.find(sha1); it != end(rom_sha1_to_index))
			return roms[it->second];

		LOG_WARN("ROM not found in DB");
		return {};
	}
}
