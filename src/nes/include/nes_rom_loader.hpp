#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string_view>
#include <vector>

#include <nes_rom.hpp>
#include <nes_types.hpp>

namespace nesem
{
	class NesRomLoader final
	{
	public:
		static NesRomLoader create(const std::filesystem::path &nes20db_file);

		NesRomLoader() = default;
		explicit NesRomLoader(std::vector<mappers::ines_2::RomData> &&roms);

		std::optional<mappers::NesRom> load_rom(const std::filesystem::path &filename) noexcept;

		std::optional<mappers::ines_2::RomData> find_rom_data(std::string_view sha1);

	private:
		void build_indexes();

	private:
		std::vector<mappers::ines_2::RomData> roms;
		std::map<std::string_view, size_t> rom_sha1_to_index;
	};
}
