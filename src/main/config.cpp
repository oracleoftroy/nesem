#define TOML_EXCEPTIONS 0
#include "config.hpp"

#include <fstream>

#include <toml++/toml.h>

#include <util/logging.hpp>

using namespace std::string_view_literals;

namespace app
{
	Config load_config_file(const std::filesystem::path &path) noexcept
	{
		Config config;

		auto result = toml::parse_file(path.string());
		if (!result)
			LOG_WARN("Could not parse config file {}, reason: {}", path.string(), result.error().description());

		else
		{
			auto table = result.table();

			auto version = table["version"].value_or(0);
			if (version == 0)
			{
				config.last_played_rom = table["last-rom"].value<std::string>();
				config.palette = table["palette"].value<std::string>();

				config.controller_1.turbo_speed = table["turbo-speed"].value_or(16);
				config.controller_1.turbo_a = table["turbo-A"].value_or(";"sv);
				config.controller_1.turbo_b = table["turbo-B"].value_or("L"sv);
				config.controller_1.a = table["A"].value_or("/"sv);
				config.controller_1.b = table["B"].value_or("."sv);
				config.controller_1.select = table["Select"].value_or(","sv);
				config.controller_1.start = table["Start"].value_or("Space"sv);
				config.controller_1.up = table["Up"].value_or("W"sv);
				config.controller_1.down = table["Down"].value_or("S"sv);
				config.controller_1.left = table["Left"].value_or("A"sv);
				config.controller_1.right = table["Right"].value_or("D"sv);
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
			table.insert("last-rom", *config.last_played_rom);

		if (config.palette)
			table.insert("palette", *config.palette);

		table.insert("controller-1",
			toml::table{
				{"turbo-speed", config.controller_1.turbo_speed},
				{"turbo-A",     config.controller_1.turbo_a    },
				{"turbo-B",     config.controller_1.turbo_b    },
				{"A",           config.controller_1.a          },
				{"B",           config.controller_1.b          },
				{"Select",      config.controller_1.select     },
				{"Start",       config.controller_1.start      },
				{"Up",          config.controller_1.up         },
				{"Down",        config.controller_1.down       },
				{"Left",        config.controller_1.left       },
				{"Right",       config.controller_1.right      },
        });

		auto file = std::ofstream(path, std::ios::trunc);
		if (!file)
			LOG_WARN("Could not open file for saving: {}", path.string());

		file << table;
	}
}
