#pragma once

#include <filesystem>
#include <optional>
#include <span>
#include <string>

namespace app
{
	struct ControllerConfig
	{
		int turbo_speed = 16;
		std::string turbo_a = ";";
		std::string turbo_b = "L";
		std::string a = "/";
		std::string b = ".";
		std::string select = ",";
		std::string start = "Space";
		std::string up = "W";
		std::string down = "S";
		std::string left = "A";
		std::string right = "D";
	};

	struct Config
	{
		std::optional<std::filesystem::path> last_played_rom;
		std::optional<std::filesystem::path> palette;
		std::optional<std::filesystem::path> nes20db_filename;
		ControllerConfig controller_1;
	};

	Config load_config_file(const std::filesystem::path &path) noexcept;
	void save_config_file(const std::filesystem::path &path, const Config &config) noexcept;

	void parse_command_line(Config &config, std::span<char *> args) noexcept;
}
