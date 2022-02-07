#include <ui/app.hpp>
#include <util/logging.hpp>
#include <util/rng.hpp>

#include "nes.hpp"

class NesApp
{
public:
	NesApp(ui::App &app)
	{
		app.on_update = std::bind_front(&NesApp::tick, this);

		toggle_fullscreen_key = app.key_from_name("F11");

		test_rom_loaded = nes.load_rom(R"(D:\Development\src\nes\data\nestest.nes)");
	}

private:
	void tick(ui::App &app, ui::Canvas &canvas, float deltatime)
	{
		handle_input(app);
		update(deltatime);
		render(canvas);
	}

	void handle_input(ui::App &app)
	{
		if (app.key_pressed(toggle_fullscreen_key))
		{
			fullscreen = !fullscreen;
			LOG_INFO("fullscreen now: {}", fullscreen);
			app.fullscreen(fullscreen);
		}
	}

	void update(double deltatime)
	{
		if (test_rom_loaded)
			nes.tick(deltatime);
	}

	void render(ui::Canvas &canvas)
	{
		canvas.fill({});

		auto size = nes_screen.size();
		auto scale = 2;

		for (int y = 0; y < size.h; ++y)
			for (int x = 0; x < size.w; ++x)
			{
				auto color = cm::Color{};

				if (rng.random_int(1) == 1)
					color = cm::Color{255, 255, 255};

				nes_screen.draw_point(color, {x, y});
			}

		canvas.blit({4, 4}, nes_screen, std::nullopt, {scale, scale});
		canvas.draw_rect({200, 200, 200}, rect({4, 4}, size * scale));
	}

	ui::Canvas nes_screen = ui::Canvas({256, 240});

	ui::Key toggle_fullscreen_key;
	bool fullscreen = false;

	bool test_rom_loaded = false;
	nesem::Nes nes;
	util::Random rng;
};

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

	auto core = NesApp{app};

	app.run();

	LOG_INFO("Exiting...");
	return 0;
}
