#include <locale>

#include <SDL2/SDL_main.h>

// the application main function... hijacked a la SDL2 to provide some default initialization
extern int application_main(int argc, char *argv[]);

extern "C" int main(int argc, char *argv[])
{
	// use the preferred locale by default, not the "C" locale
	std::locale::global(std::locale(""));

	return application_main(argc, argv);
}
