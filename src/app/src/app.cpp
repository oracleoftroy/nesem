#include "ui/app.hpp"

#include <chrono>
#include <locale>
#include <type_traits>
#include <utility>

#include <SDL2/SDL.h>
#include <util/logging.hpp>
#include <util/ptr.hpp>

namespace ui
{
	using SdlWindow = util::custom_unique_ptr<SDL_Window, &SDL_DestroyWindow>;
	using SdlSurface = util::custom_unique_ptr<SDL_Surface, &SDL_FreeSurface>;

	struct Clock
	{
		using clock = std::chrono::steady_clock;
		clock::time_point current_time = clock::now();

		std::chrono::duration<double> update() noexcept
		{
			auto last_time = std::exchange(current_time, clock::now());
			return current_time - last_time;
		}
	};

	class InputState
	{
	public:
		InputState();
		void update(cm::Sizei window_size, cm::Sizei canvas_size) noexcept;

		[[nodiscard]] KeyMods modifiers() const noexcept;
		[[nodiscard]] bool key_down(Key key) const noexcept;
		[[nodiscard]] bool key_down(Scancode code) const noexcept;
		[[nodiscard]] bool key_up(Key key) const noexcept;
		[[nodiscard]] bool key_up(Scancode code) const noexcept;
		[[nodiscard]] bool key_pressed(Key key) const noexcept;
		[[nodiscard]] bool key_pressed(Scancode code) const noexcept;
		[[nodiscard]] bool key_released(Key key) const noexcept;
		[[nodiscard]] bool key_released(Scancode code) const noexcept;
		[[nodiscard]] bool mouse_down(int button) const noexcept;
		[[nodiscard]] bool mouse_up(int button) const noexcept;
		[[nodiscard]] bool mouse_pressed(int button) const noexcept;
		[[nodiscard]] bool mouse_released(int button) const noexcept;
		[[nodiscard]] cm::Point2i mouse_position() const noexcept;

	private:
		std::vector<uint8_t> last_keys;
		std::vector<uint8_t> current_keys;
		KeyMods mods = KeyMods::none;

		cm::Point2i mouse_pos{};
		uint32_t last_mouse_buttons = 0;
		uint32_t current_mouse_buttons = 0;
	};

	struct FPS
	{
		double fastest_frame = std::numeric_limits<double>::infinity();
		double slowest_frame = 0.0;
		double accum_time = 0.0;
		uint32_t framecount = 0;

		void update(double deltatime) noexcept
		{
			// skip the first frame after a reset (when we do extra work to display fps data)
			if (framecount != 0)
			{
				fastest_frame = std::min(fastest_frame, deltatime);
				slowest_frame = std::max(slowest_frame, deltatime);

				accum_time += deltatime;
			}

			++framecount;
		}

		[[nodiscard]] double average() const noexcept
		{
			// subtract 1 to account for skipped frame
			return (framecount - 1) / accum_time;
		}

		[[nodiscard]] double low() const noexcept
		{
			return 1 / slowest_frame;
		}

		[[nodiscard]] double high() const noexcept
		{
			return 1 / fastest_frame;
		}

		void reset() noexcept
		{
			fastest_frame = std::numeric_limits<double>::infinity();
			slowest_frame = 0.0;
			accum_time = 0.0;
			framecount = 0;
		}
	};

	struct SdlLib
	{
		static SdlLib init(uint32_t flags = SDL_INIT_EVERYTHING)
		{
			if (SDL_Init(flags) != 0)
			{
				LOG_CRITICAL("Error initializing SDL: {}", SDL_GetError());
				return SdlLib{};
			}

			SDL_version version;
			SDL_GetVersion(&version);
			LOG_INFO("SDL Version {}.{}.{} {}", version.major, version.minor, version.patch, SDL_GetRevision());

			return SdlLib{true};
		}

		~SdlLib()
		{
			if (is_initialized)
			{
				LOG_INFO("Quitting SDL");
				SDL_Quit();
			}
		}

		// default move is fine
		SdlLib(SdlLib &&other) noexcept
			: is_initialized(std::exchange(other.is_initialized, false))
		{
		}

		SdlLib &operator=(SdlLib &&other) noexcept
		{
			is_initialized = std::exchange(other.is_initialized, false);
			return *this;
		}

		// only one copy should exist
		SdlLib(const SdlLib &other) noexcept = delete;
		SdlLib &operator=(const SdlLib &other) noexcept = delete;

	private:
		// don't allow clients to default construct, must use init()
		explicit SdlLib(bool is_initialized = false) noexcept
			: is_initialized(is_initialized)
		{
		}

		bool is_initialized = false;
	};

	struct App::Core
	{
		SdlLib sdl;
		std::string window_title;
		cm::Sizei window_size;

		Canvas canvas;
		SdlWindow window;
		SDL_Surface *window_surface;
		SdlSurface canvas_surface;

		InputState input;
	};

	App App::create(const std::string &window_title, cm::Sizei window_size, cm::Sizei pixel_size) noexcept
	{
		auto sdl = SdlLib::init(SDL_INIT_VIDEO);

		auto canvas_size = window_size / pixel_size;
		auto canvas = Canvas(canvas_size);
		auto format = canvas.format();

		auto window = SdlWindow(SDL_CreateWindow(window_title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_size.w, window_size.h, SDL_WINDOW_ALLOW_HIGHDPI));
		if (!window)
		{
			LOG_CRITICAL("Error creating window: {}", SDL_GetError());
			return {};
		}

		auto window_surface = SDL_GetWindowSurface(window.get());
		if (!window_surface)
		{
			LOG_CRITICAL("Error getting window surface: {}", SDL_GetError());
			return {};
		}

		LOG_INFO(
			"Window surface format:\n"
			"    R mask: 0x{:08X}\n"
			"    G mask: 0x{:08X}\n"
			"    B mask: 0x{:08X}\n"
			"    A mask: 0x{:08X}",
			window_surface->format->Rmask,
			window_surface->format->Gmask,
			window_surface->format->Bmask,
			window_surface->format->Amask);

		// Create the SDL surface without alpha. All blending will be done by the canvas, and
		// SDL is very slow if an alpha component is provided, even when blending is disabled
		auto canvas_surface = SdlSurface(SDL_CreateRGBSurfaceFrom(canvas.ptr(),
			canvas_size.w, canvas_size.h, 32, canvas_size.w * sizeof(*canvas.ptr()),
			format.mask_r, format.mask_g, format.mask_b, 0));

		if (!canvas_surface)
		{
			LOG_CRITICAL("Error creating canvas surface: {}", SDL_GetError());
			return {};
		}

		return App{std::make_unique<Core>(
			std::move(sdl),
			std::move(window_title),
			window_size,
			std::move(canvas),
			std::move(window),
			window_surface,
			std::move(canvas_surface))};
	}

	App::App(std::unique_ptr<Core> &&core) noexcept
		: core(std::move(core))
	{
	}

	void App::run()
	{
		auto process_events = [this] {
			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				switch (event.type)
				{
				case SDL_QUIT:
					return false;

				case SDL_WINDOWEVENT:
					if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
					{
						LOG_INFO("Window size changed, now {}x{}", event.window.data1, event.window.data2);

						// changing size invalidates the surface, so reacquire
						if (core->window_surface = SDL_GetWindowSurface(core->window.get());
							!core->window_surface)
							LOG_CRITICAL("Error getting window surface: {}", SDL_GetError());
					}
					break;

				case SDL_DROPFILE:
					if (on_file_drop)
						on_file_drop(event.drop.file);

					// these events conveniently allocate memory for your app and leave it up to you to free. And they
					// are enabled by default. Yay! Free memory leak unless you explicitly handle or disable them! :(
					SDL_free(event.drop.file);
					break;

				case SDL_DROPTEXT:
					// these events conveniently allocate memory for your app and leave it up to you to free. And they
					// are enabled by default. Yay! Free memory leak unless you explicitly handle or disable them! :(
					SDL_free(event.drop.file);
					break;
				}
			}

			return true;
		};

		constexpr auto frame_time = 1 / 60.0f;
		double elapsed_time = 0.0;
		Clock clock;
		FPS fps;

		while (process_events())
		{
			// tick
			auto real_deltatime = clock.update().count();
			fps.update(real_deltatime);

			// if we have a large frameskip, cap it for the fixed timestep
			auto deltatime = std::min(real_deltatime, 0.25);
			elapsed_time += deltatime;

			while (elapsed_time > frame_time)
			{
				elapsed_time -= frame_time;

				if (fps.accum_time >= 1.0)
				{
					SDL_SetWindowTitle(core->window.get(), fmt::format("{} - FPS: {:.2f} Low: {:.2f} High: {:.2f}", core->window_title, fps.average(), fps.low(), fps.high()).c_str());
					fps.reset();
				}

				core->input.update(core->window_size, core->canvas.size());
				if (on_update)
					on_update(*this, core->canvas, frame_time);
			}

			// render
			if (SDL_BlitScaled(core->canvas_surface.get(), nullptr, core->window_surface, nullptr) != 0)
				LOG_WARN("Blit failed, reason: {}", SDL_GetError());

			if (SDL_UpdateWindowSurface(core->window.get()) != 0)
			{
				LOG_WARN("Updating window failed, reason: {}", SDL_GetError());

				if (core->window_surface = SDL_GetWindowSurface(core->window.get());
					!core->window_surface)
					LOG_CRITICAL("Error getting window surface: {}", SDL_GetError());
			}
		}
	}

	void App::fullscreen(bool mode) noexcept
	{
		int flags = 0;
		if (mode)
			flags = SDL_WINDOW_FULLSCREEN_DESKTOP;

		if (SDL_SetWindowFullscreen(core->window.get(), flags) != 0)
			LOG_WARN("Could not set fullscreen to {}, reason: {}", mode, SDL_GetError());

		core->window_surface = SDL_GetWindowSurface(core->window.get());
		if (!core->window_surface)
			LOG_WARN("Could not acquire window surface, reason: {}", SDL_GetError());

		// update our window size to reflect the change
		SDL_GetWindowSize(core->window.get(), &core->window_size.w, &core->window_size.h);
	}

	App::operator bool() noexcept
	{
		return !!core;
	}

	Key App::key_from_name(const char *name) noexcept
	{
		auto key = SDL_GetKeyFromName(name);

		if (key == SDLK_UNKNOWN)
			LOG_WARN("Could not find key named '{}', reason: {}", name, SDL_GetError());

		return Key{key};
	}

	Scancode App::scancode_from_name(const char *name) noexcept
	{
		auto scancode = SDL_GetScancodeFromName(name);
		if (scancode == SDL_SCANCODE_UNKNOWN)
			LOG_WARN("Could not find scancode named '{}', reason: {}", name, SDL_GetError());

		return Scancode{scancode};
	}

	KeyMods App::modifiers() const noexcept
	{
		return core->input.modifiers();
	}

	bool App::key_down(Key key) const noexcept
	{
		return core->input.key_down(key);
	}

	bool App::key_down(Scancode code) const noexcept
	{
		return core->input.key_down(code);
	}

	bool App::key_up(Key key) const noexcept
	{
		return core->input.key_up(key);
	}

	bool App::key_up(Scancode code) const noexcept
	{
		return core->input.key_up(code);
	}

	bool App::key_pressed(Key key) const noexcept
	{
		return core->input.key_pressed(key);
	}

	bool App::key_pressed(Scancode code) const noexcept
	{
		return core->input.key_pressed(code);
	}

	bool App::key_released(Key key) const noexcept
	{
		return core->input.key_released(key);
	}

	bool App::key_released(Scancode code) const noexcept
	{
		return core->input.key_released(code);
	}

	bool App::mouse_down(int button) const noexcept
	{
		return core->input.mouse_down(button);
	}

	bool App::mouse_up(int button) const noexcept
	{
		return core->input.mouse_up(button);
	}

	bool App::mouse_pressed(int button) const noexcept
	{
		return core->input.mouse_pressed(button);
	}

	bool App::mouse_released(int button) const noexcept
	{
		return core->input.mouse_released(button);
	}

	cm::Point2i App::mouse_position() const noexcept
	{
		return core->input.mouse_position();
	}

	cm::Sizei App::canvas_size() const noexcept
	{
		return core->canvas.size();
	}

	App::App() noexcept = default;
	App::~App() = default;

	InputState::InputState()
	{
		// sanity check that our assumptions about SDL are valid

		// theses don't technically need to be exactly the same type as long as X::value is big enough to hold any key or scancode
		// but both seem to be ints, so we'll assume that will stay the same for now
		static_assert(std::is_same_v<decltype(Key::value), SDL_Keycode>);
		static_assert(sizeof(Scancode) == sizeof(SDL_Scancode));

		// ideally, Scancode::value would be the same type as the SDL_Scancode enum, but for some reason, gcc 11 on
		// linux chooses an unsigned int representation and msvc a signed int. SDL doesn't
		// static_assert(std::is_same_v<decltype(Key::value), std::underlying_type_t<SDL_Scancode>>);
		static_assert(std::is_nothrow_convertible_v<decltype(Scancode::value), std::underlying_type_t<SDL_Scancode>>);

		// we assume 0 is used as the unknown/invalid key. SDL2 probably won't change that, but good to sanity check in case SDL3+ makes major changes
		static_assert(SDLK_UNKNOWN == 0);
		static_assert(SDL_SCANCODE_UNKNOWN == 0);

		// KeyMods should directly map to SDL_KeyMod
		static_assert(util::to_underlying(KeyMods::none) == KMOD_NONE);
		static_assert(util::to_underlying(KeyMods::none) == KMOD_NONE);
		static_assert(util::to_underlying(KeyMods::left_shift) == KMOD_LSHIFT);
		static_assert(util::to_underlying(KeyMods::right_shift) == KMOD_RSHIFT);
		static_assert(util::to_underlying(KeyMods::left_ctrl) == KMOD_LCTRL);
		static_assert(util::to_underlying(KeyMods::right_ctrl) == KMOD_RCTRL);
		static_assert(util::to_underlying(KeyMods::left_alt) == KMOD_LALT);
		static_assert(util::to_underlying(KeyMods::right_alt) == KMOD_RALT);
		static_assert(util::to_underlying(KeyMods::left_gui) == KMOD_LGUI);
		static_assert(util::to_underlying(KeyMods::right_gui) == KMOD_RGUI);
		static_assert(util::to_underlying(KeyMods::numlock) == KMOD_NUM);
		static_assert(util::to_underlying(KeyMods::capslock) == KMOD_CAPS);
		static_assert(util::to_underlying(KeyMods::alt_gr) == KMOD_MODE);
		static_assert(util::to_underlying(KeyMods::ctrl) == KMOD_CTRL);
		static_assert(util::to_underlying(KeyMods::shift) == KMOD_SHIFT);
		static_assert(util::to_underlying(KeyMods::alt) == KMOD_ALT);
		static_assert(util::to_underlying(KeyMods::gui) == KMOD_GUI);

		// the number of keys on a keyboard shouldn't change, so size our vectors now while we aren't on a hot path
		int num_keys;
		SDL_GetKeyboardState(&num_keys);

		last_keys.resize(num_keys);
		current_keys.resize(num_keys);
	}

	void InputState::update(cm::Sizei window_size, cm::Sizei canvas_size) noexcept
	{
		int num_keys;
		auto keys = SDL_GetKeyboardState(&num_keys);

		// swap and assign should avoid having to reallocate every frame and encourage memory reuse
		std::swap(last_keys, current_keys);
		current_keys.assign(keys, keys + num_keys);

		mods = static_cast<KeyMods>(SDL_GetModState());

		// update mouse position and buttons
		last_mouse_buttons = std::exchange(current_mouse_buttons, SDL_GetMouseState(&mouse_pos.x, &mouse_pos.y));
		mouse_pos = (mouse_pos * canvas_size) / window_size;
	}

	bool InputState::key_down(Key key) const noexcept
	{
		if (!key)
			LOG_WARN("Invalid key");

		return key_down(Scancode{SDL_GetScancodeFromKey(key.value)});
	}

	bool InputState::key_up(Key key) const noexcept
	{
		if (!key)
			LOG_WARN("Invalid key");

		return key_up(Scancode{SDL_GetScancodeFromKey(key.value)});
	}

	bool InputState::key_pressed(Key key) const noexcept
	{
		if (!key)
			LOG_WARN("Invalid key");

		return key_pressed(Scancode{SDL_GetScancodeFromKey(key.value)});
	}

	bool InputState::key_released(Key key) const noexcept
	{
		if (!key)
			LOG_WARN("Invalid key");

		return key_released(Scancode{SDL_GetScancodeFromKey(key.value)});
	}

	bool InputState::key_down(Scancode scancode) const noexcept
	{
		if (!scancode)
			LOG_WARN("Invalid scancode");

		if (scancode.value < 0 || std::cmp_greater(scancode.value, size(current_keys)))
		{
			LOG_WARN("Key is out of range: {}", scancode.value);
			return false;
		}

		return current_keys[scancode.value];
	}

	bool InputState::key_up(Scancode scancode) const noexcept
	{
		if (!scancode)
			LOG_WARN("Invalid scancode");

		if (scancode.value < 0 || std::cmp_greater(scancode.value, size(current_keys)))
		{
			LOG_WARN("Key is out of range: {}", scancode.value);
			return false;
		}

		return !current_keys[scancode.value];
	}

	bool InputState::key_pressed(Scancode scancode) const noexcept
	{
		if (!scancode)
			LOG_WARN("Invalid scancode");

		if (scancode.value < 0 || std::cmp_greater_equal(scancode.value, size(last_keys)))
		{
			LOG_WARN("Key {} is out of range of last_keys", scancode.value);
			return false;
		}

		// 'pressed' is the transition from up to down, so just keys
		// that were up last frame but are now down return true
		return !last_keys[scancode.value] && key_down(scancode);
	}

	bool InputState::key_released(Scancode scancode) const noexcept
	{
		if (!scancode)
			LOG_WARN("Invalid scancode");

		if (scancode.value < 0 || std::cmp_greater_equal(scancode.value, size(last_keys)))
		{
			LOG_WARN("Key {} is out of range of last_keys", scancode.value);
			return false;
		}

		// 'released' is the transition from down to up, so just keys
		// that were down last frame but are now up return true
		return last_keys[scancode.value] && key_up(scancode);
	}

	KeyMods InputState::modifiers() const noexcept
	{
		return mods;
	}

	bool InputState::mouse_down(int button) const noexcept
	{
		if (button == 0 || button > 32)
		{
			LOG_WARN("Mouse button {} does not exist", button);
			return false;
		}

		return current_mouse_buttons & SDL_BUTTON(button);
	}

	bool InputState::mouse_up(int button) const noexcept
	{
		if (button == 0 || button > 32)
		{
			LOG_WARN("Mouse button {} does not exist", button);
			return false;
		}

		return (current_mouse_buttons & SDL_BUTTON(button)) == 0;
	}

	bool InputState::mouse_pressed(int button) const noexcept
	{
		if (button == 0 || button > 32)
		{
			LOG_WARN("Mouse button {} does not exist", button);
			return false;
		}

		return !(last_mouse_buttons & SDL_BUTTON(button)) && mouse_down(button);
	}

	bool InputState::mouse_released(int button) const noexcept
	{
		if (button == 0 || button > 32)
		{
			LOG_WARN("Mouse button {} does not exist", button);
			return false;
		}

		return (last_mouse_buttons & SDL_BUTTON(button)) && mouse_up(button);
	}

	cm::Point2i InputState::mouse_position() const noexcept
	{
		return mouse_pos;
	}
}
