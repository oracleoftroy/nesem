#include <filesystem>
#include <fstream>
#include <locale>

#include "config.hpp"
#include "nes_app.hpp"

#include <ui/app.hpp>
#include <util/logging.hpp>

const static auto logger_init = [] {
	// use the preferred locale by default, not the "C" locale
	std::locale::global(std::locale(""));

	const auto log_file_name = std::filesystem::path("nesem.log");
	auto base_path = ui::App::get_user_data_path("nesem");

	return util::detail::LoggerInit{base_path / log_file_name};
}();

int application_main(int argc, char *argv[])
{
	LOG_INFO("Starting: {}", fmt::join(argv, argv + argc, " "));
	LOG_INFO("Working directory: {}", std::filesystem::current_path().string());

	constexpr auto window_size = cm::Size{1024, 768};

	auto app = ui::App::create("NES emulator", window_size);

	if (!app)
	{
		LOG_ERROR("Error creating app, exiting...");
		return -1;
	}

	auto config_path = ui::App::get_user_data_path("nesem") / "nesem.toml";
	auto config = app::load_config_file(config_path);

	auto core = app::NesApp{app, config};

	app.run();

	app::save_config_file(config_path, core.get_config(app));

	LOG_INFO("Exiting...");
	return 0;
}
