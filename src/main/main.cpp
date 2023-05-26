#include <filesystem>
#include <fstream>
#include <locale>
#include <memory>
#include <string_view>
#include <vector>

#include "config.hpp"
#include "nes_app.hpp"

#include <ui/app.hpp>
#include <util/logging.hpp>

#if defined(__EMSCRIPTEN__)
#	include <emscripten.h>
#	include <emscripten/html5.h>

struct RuntimeData
{
	RuntimeData(const std::string &title, cm::Sizei window_size, const app::Config &config)
		: app{ui::App::create(title, window_size)}, core(app, config)
	{
	}

	ui::App app;
	app::NesApp core;
};

void run_once(void *user_data)
{
	auto data = std::unique_ptr<RuntimeData>{static_cast<RuntimeData *>(user_data)};

	if (!data || !data->app.run_once())
	{
		// loop finished retain ownership of the pointer so that it gets properly deleted
		emscripten_cancel_main_loop();
		return;
	}

	// loop continuing, release ownership of the pointer until next tick
	data.release();
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

#if defined(__EMSCRIPTEN__)
	auto runtime_data = std::make_unique<RuntimeData>("NES emulator", window_size, app::Config{});

	LOG_INFO("starting wasm loop...");
	emscripten_set_main_loop_arg(run_once, runtime_data.release(), 0, false);
#else
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

	app::save_config_file(config_path, core.get_config());
	LOG_INFO("Exiting...");
#endif

	return 0;
}
