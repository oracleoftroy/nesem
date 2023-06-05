#include <algorithm>
#include <filesystem>
#include <fstream>
#include <locale>
#include <memory>
#include <string_view>
#include <vector>

#include "config.hpp"
#include "nes_app.hpp"

#include <util/logging.hpp>

#if defined(__EMSCRIPTEN__)
#	include <emscripten.h>

void run_once(void *user_data)
{
	auto app = std::unique_ptr<app::NesApp>{static_cast<app::NesApp *>(user_data)};

	if (!app || !app->tick())
	{
		// loop finished retain ownership of the pointer so that it gets properly deleted
		emscripten_cancel_main_loop();
		return;
	}

	// loop continuing, release ownership of the pointer until next tick
	app.release();
}

#endif

const static auto logger_init = [] {
	// use the preferred locale by default, not the "C" locale
	std::locale::global(std::locale(""));

	const auto log_file_name =
#if defined(__EMSCRIPTEN__)
		std::filesystem::path{};
#else
		ui::App::get_user_data_path("nesem") / std::filesystem::path("nesem.log");
#endif

	return util::detail::LoggerInit{log_file_name};
}();

constexpr auto args(int argc, char *argv[])
{
	return std::vector<std::string_view>(argv, argv + argc);
}

void init([[maybe_unused]] const std::filesystem::path &config_path)
{
#if defined(__EMSCRIPTEN__)
// temporarily disable false positive warning
// EM_ASM blocks are javascript, not C++ and $n is the name emscripten uses for parameters
// TODO: can we make a NESEM_ASM macro to wrap suppressing this warning?
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wdollar-in-identifier-extension"
	EM_ASM(
		{
			// mount config_path
			FS.mount(IDBFS, {}, UTF8ToString($0));

			// sync from persisted state into memory and then
		    // run the 'test' function
			FS.syncfs(
				true, function(err) {
					assert(!err);
				});
		},
		config_path.string().c_str());
#	pragma GCC diagnostic pop

#endif
}

int application_main(int argc, char *argv[])
{
	// fmt regression? This no longer works in fmt v10
	// LOG_INFO("Starting: {}", fmt::join(argv, argv + argc, " "));

	LOG_INFO("Starting: {}", fmt::join(args(argc, argv), " "));
	LOG_INFO("Working directory: {}", std::filesystem::current_path().string());

	auto config_path = ui::App::get_user_data_path("nesem");
	LOG_INFO("Config directory: {}", config_path.string());

	init(config_path);

	auto config_file_path = config_path / "nesem.toml";

	auto config = app::load_config_file(config_file_path);
	parse_command_line(config, {argv, argv + argc});
	auto app = std::make_unique<app::NesApp>(config);

#if defined(__EMSCRIPTEN__)

	emscripten_set_main_loop_arg(run_once, app.release(), 0, false);

// documentation on this is very sparse. I'm not clear if this is only for SDL1, if I need to enable
// certain compile options, or what, but unconditionally setting this produces an error
// moreover, when can/should this be called? before SDL_Init()? before creating a window and or renderer? Is after OK?
#	ifdef TEST_SDL_LOCK_OPTS
	LOG_INFO("Setting SDL options");
	EM_ASM(
		SDL.defaults.copyOnLock = false;
		SDL.defaults.discardOnLock = true;
		SDL.defaults.opaqueFrontBuffer = false;);
#	endif
#else

	while (app->tick())
	{
		// nothing
	}

	app::save_config_file(config_file_path, app->get_config());
	LOG_INFO("Exiting...");
#endif

	return 0;
}
