#include <filesystem>
#include <fstream>
#include <locale>

#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>

#include <util/logging.hpp>

// the application main function... hijacked a la SDL2 to provide some default initialization
extern int application_main(int argc, char *argv[]);

util::detail::LoggerInit configure_logger()
{
	namespace fs = std::filesystem;
	const auto log_file_name = fs::path("nesem.log");

	fs::path log_file_path;

	if (auto base_path = SDL_GetPrefPath(nullptr, "nesem");
		base_path)
		log_file_path = fs::path(base_path) / log_file_name;

	return util::detail::LoggerInit{log_file_path};
}

extern "C" int main(int argc, char *argv[])
{
	// use the preferred locale by default, not the "C" locale
	std::locale::global(std::locale(""));

	util::detail::LoggerInit logger_init = configure_logger();

	return application_main(argc, argv);
}
