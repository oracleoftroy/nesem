
#include <filesystem>

#include "nes_app.hpp"

#include <ui/app.hpp>
#include <util/logging.hpp>

int main(int argc, char *argv[])
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

	auto core = app::NesApp{app};

	app.run();

	LOG_INFO("Exiting...");
	return 0;
}
