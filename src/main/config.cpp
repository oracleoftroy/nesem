#define TOML_EXCEPTIONS 0
#include "config.hpp"

#include <fstream>

#include <toml++/toml.h>

#include <util/logging.hpp>

using namespace std::string_view_literals;

namespace app
{
	constinit auto key_version = "version"sv;
	constinit auto key_last_rom = "last-rom"sv;
	constinit auto key_palette = "palette"sv;

	constinit auto key_controller_1 = "controller-1"sv;
	constinit auto key_turbo_speed = "turbo-speed"sv;
	constinit auto key_turbo_a = "turbo-A"sv;
	constinit auto key_turbo_b = "turbo-B"sv;
	constinit auto key_a = "A"sv;
	constinit auto key_b = "B"sv;
	constinit auto key_select = "Select"sv;
	constinit auto key_start = "Start"sv;
	constinit auto key_up = "Up"sv;
	constinit auto key_down = "Down"sv;
	constinit auto key_left = "Left"sv;
	constinit auto key_right = "Right"sv;

	Config load_config_file(const std::filesystem::path &path) noexcept
	{
		Config config;

		auto result = toml::parse_file(path.string());
		if (!result)
			LOG_WARN("Could not parse config file {}, reason: {}", path.string(), result.error().description());

		else
		{
			auto table = result.table();

			auto version = table[key_version].value_or(0);
			if (version == 0)
			{
				config.last_played_rom = table[key_last_rom].value<std::string>();
				config.palette = table[key_palette].value<std::string>();

				auto controller_1 = table[key_controller_1];
				config.controller_1.turbo_speed = controller_1[key_turbo_speed].value_or(16);
				config.controller_1.turbo_a = controller_1[key_turbo_a].value_or(";"sv);
				config.controller_1.turbo_b = controller_1[key_turbo_b].value_or("L"sv);
				config.controller_1.a = controller_1[key_a].value_or("/"sv);
				config.controller_1.b = controller_1[key_b].value_or("."sv);
				config.controller_1.select = controller_1[key_select].value_or(","sv);
				config.controller_1.start = controller_1[key_start].value_or("Space"sv);
				config.controller_1.up = controller_1[key_up].value_or("W"sv);
				config.controller_1.down = controller_1[key_down].value_or("S"sv);
				config.controller_1.left = controller_1[key_left].value_or("A"sv);
				config.controller_1.right = controller_1[key_right].value_or("D"sv);
			}
			else
				LOG_WARN("Unexpected config file version: {}", version);
		}

		return config;
	}

	void save_config_file(const std::filesystem::path &path, const Config &config) noexcept
	{
		auto table = toml::table{};

		if (config.last_played_rom)
			table.insert(key_last_rom, *config.last_played_rom);

		if (config.palette)
			table.insert(key_palette, *config.palette);

		table.insert("controller-1"sv,
			toml::table{
				{key_turbo_speed, config.controller_1.turbo_speed},
				{key_turbo_a, config.controller_1.turbo_a},
				{key_turbo_b, config.controller_1.turbo_b},
				{key_a, config.controller_1.a},
				{key_b, config.controller_1.b},
				{key_select, config.controller_1.select},
				{key_start, config.controller_1.start},
				{key_up, config.controller_1.up},
				{key_down, config.controller_1.down},
				{key_left, config.controller_1.left},
				{key_right, config.controller_1.right},
			});

		auto file = std::ofstream(path, std::ios::trunc);
		if (!file)
			LOG_WARN("Could not open file for saving: {}", path.string());

		file << table;
	}
}
