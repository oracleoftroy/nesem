#include <filesystem>
#include <fstream>
#include <locale>
#include <string_view>
#include <vector>

#include "config.hpp"
#include "nes_app.hpp"

#include <ui/app.hpp>
#include <util/logging.hpp>

#if defined(__EMSCRIPTEN__)
#	include <emscripten.h>
#	include <emscripten/html5.h>

void run_once(void *user_data)
{
	auto *app = static_cast<ui::App *>(user_data);

	if (!app || !app->run_once())
		emscripten_cancel_main_loop();
}

#endif

const static auto logger_init = [] {
	// use the preferred locale by default, not the "C" locale
	std::locale::global(std::locale(""));

	const auto log_file_name = std::filesystem::path("nesem.log");
	auto base_path = ui::App::get_user_data_path("nesem");

	return util::detail::LoggerInit{base_path / log_file_name};
}();

constexpr auto args(int argc, char *argv[])
{
	return std::vector<std::string_view>(argv, argv + argc);
}

int application_main(int argc, char *argv[])
{
	// fmt regression? This no longer works in fmt v10
	// LOG_INFO("Starting: {}", fmt::join(argv, argv + argc, " "));

	LOG_INFO("Starting: {}", fmt::join(args(argc, argv), " "));
	LOG_INFO("Working directory: {}", std::filesystem::current_path().string());

	constexpr auto window_size = cm::Size{1024, 768};

	auto app = ui::App::create("NES emulator", window_size);

	if (!app)
	{
		LOG_ERROR("Error creating app, exiting...");
		return -1;
	}

#if defined(__EMSCRIPTEN__)
	auto core = app::NesApp{app, app::Config{}};
	emscripten_set_main_loop_arg(run_once, &app, 0, false);
#else
	auto config_path = ui::App::get_user_data_path("nesem") / "nesem.toml";
	auto config = app::load_config_file(config_path);

	auto core = app::NesApp{app, config};

	app.run();

	app::save_config_file(config_path, core.get_config());
#endif

	LOG_INFO("Exiting...");
	return 0;
}
