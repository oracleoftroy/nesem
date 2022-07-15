#include "ui/app.hpp"

#include <algorithm>
#include <chrono>
#include <locale>
#include <numeric>
#include <type_traits>
#include <utility>
#include <vector>

#include <SDL2/SDL.h>

#include <ui/audio_device.hpp>
#include <ui/renderer.hpp>
#include <ui/texture.hpp>
#include <util/logging.hpp>
#include <util/ptr.hpp>

namespace ui
{
	namespace
	{
		cm::Recti viewport(SDL_Renderer *renderer) noexcept
		{
			SDL_Rect rect;
			SDL_RenderGetViewport(renderer, &rect);

			return cm::Rect{
				.x = rect.x,
				.y = rect.y,
				.w = rect.w,
				.h = rect.h,
			};
		}
	}

	using SdlWindow = util::custom_unique_ptr<SDL_Window, &SDL_DestroyWindow>;
	using SdlSurface = util::custom_unique_ptr<SDL_Surface, &SDL_FreeSurface>;
	using SdlRenderer = util::custom_unique_ptr<SDL_Renderer, &SDL_DestroyRenderer>;

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

	class InputState final
	{
	public:
		InputState();

		void update(const App::Core &core) noexcept;

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

	class FPS
	{
	public:
		void update(double deltatime) noexcept
		{
			// skip the first frame after a reset (when we do extra work to display fps data)
			if (framecount != 0)
			{
				fastest_frame = std::min(fastest_frame, deltatime);
				slowest_frame = std::max(slowest_frame, deltatime);

				accum_time += deltatime;
				times.emplace_back(deltatime);
			}

			++framecount;
		}

		[[nodiscard]] double accumulated_time() const noexcept
		{
			return accum_time;
		}

		[[nodiscard]] double fps() const noexcept
		{
			// subtract 1 to account for skipped frame
			return (framecount - 1) / accum_time;
		}

		[[nodiscard]] double average() const noexcept
		{
			return accum_time / (framecount - 1) * 1000.0;
		}

		[[nodiscard]] double median() noexcept
		{
			if (times.empty())
				return 0;

			auto size_times = size(times);

			// not a true median, but good enough. This will favor slightly higher values on an even number of samples
			auto mid = begin(times) + size_times / 2;
			std::ranges::nth_element(times, mid);

			return *mid * 1000.0;
		}

		[[nodiscard]] double worst() const noexcept
		{
			return slowest_frame * 1000.0;
		}

		[[nodiscard]] double best() const noexcept
		{
			return fastest_frame * 1000.0;
		}

		void reset() noexcept
		{
			fastest_frame = std::numeric_limits<double>::infinity();
			slowest_frame = 0.0;
			accum_time = 0.0;
			framecount = 0;
			times.clear();
		}

	private:
		double fastest_frame = std::numeric_limits<double>::infinity();
		double slowest_frame = 0.0;
		double accum_time = 0.0;
		uint32_t framecount = 0;
		std::vector<double> times;
	};

	struct SdlLib
	{
		static SdlLib init(uint32_t flags = SDL_INIT_EVERYTHING)
		{
			SDL_LogSetOutputFunction(on_sdl_message, nullptr);

			if (SDL_Init(flags) != 0)
			{
				LOG_CRITICAL("Error initializing SDL: {}", SDL_GetError());
				return SdlLib{};
			}

			return SdlLib{true};
		}

		static void on_sdl_message([[maybe_unused]] void *userdata, int category, SDL_LogPriority priority, const char *message)
		{
			std::string_view category_str;
			switch (category)
			{
			case SDL_LOG_CATEGORY_APPLICATION:
				category_str = "application";
				break;

			case SDL_LOG_CATEGORY_ERROR:
				category_str = "error";
				break;

			case SDL_LOG_CATEGORY_ASSERT:
				category_str = "assert";
				break;

			case SDL_LOG_CATEGORY_SYSTEM:
				category_str = "system";
				break;

			case SDL_LOG_CATEGORY_AUDIO:
				category_str = "audio";
				break;

			case SDL_LOG_CATEGORY_VIDEO:
				category_str = "video";
				break;

			case SDL_LOG_CATEGORY_RENDER:
				category_str = "render";
				break;

			case SDL_LOG_CATEGORY_INPUT:
				category_str = "input";
				break;

			case SDL_LOG_CATEGORY_TEST:
				category_str = "test";
				break;

			case SDL_LOG_CATEGORY_RESERVED1:
			case SDL_LOG_CATEGORY_RESERVED2:
			case SDL_LOG_CATEGORY_RESERVED3:
			case SDL_LOG_CATEGORY_RESERVED4:
			case SDL_LOG_CATEGORY_RESERVED5:
			case SDL_LOG_CATEGORY_RESERVED6:
			case SDL_LOG_CATEGORY_RESERVED7:
			case SDL_LOG_CATEGORY_RESERVED8:
			case SDL_LOG_CATEGORY_RESERVED9:
			case SDL_LOG_CATEGORY_RESERVED10:
				category_str = "reserved";
				break;

			case SDL_LOG_CATEGORY_CUSTOM:
			default: // all other categories should fall in the custom range
				category_str = "custom";
				break;
			}

			auto level = LOG_LEVEL_DISABLED;
			switch (priority)
			{
			case SDL_LOG_PRIORITY_VERBOSE:
				level = LOG_LEVEL_TRACE;
				break;

			case SDL_LOG_PRIORITY_DEBUG:
				level = LOG_LEVEL_DEBUG;
				break;

			case SDL_LOG_PRIORITY_INFO:
				level = LOG_LEVEL_INFO;
				break;

			case SDL_LOG_PRIORITY_WARN:
				level = LOG_LEVEL_WARN;
				break;

			case SDL_LOG_PRIORITY_ERROR:
				level = LOG_LEVEL_ERROR;
				break;

			case SDL_LOG_PRIORITY_CRITICAL:
				level = LOG_LEVEL_CRITICAL;
				break;
			}

			LOG(level, "SDL {}: {}", category_str, message);
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
		cm::Sizei logical_size;
		cm::Recti renderer_viewport;

		SdlWindow window;
		SdlRenderer renderer;

		InputState input;
	};

	App App::create(const std::string &window_title, cm::Sizei window_size, cm::Sizei pixel_size) noexcept
	{
		{
			SDL_version version;
			SDL_GetVersion(&version);
			LOG_INFO("SDL Version {}.{}.{} {}", version.major, version.minor, version.patch, SDL_GetRevision());
		}

		for (int i = 0, len = SDL_GetNumVideoDrivers(); i != len; ++i)
		{
			LOG_INFO("SDL Video driver #{}: {}", i, SDL_GetVideoDriver(i));
		}

		auto sdl = SdlLib::init();

		LOG_INFO("Using video driver: {}", SDL_GetCurrentVideoDriver());

		auto logical_size = window_size / pixel_size;

		auto window = SdlWindow(SDL_CreateWindow(window_title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_size.w, window_size.h, SDL_WINDOW_ALLOW_HIGHDPI));
		if (!window)
		{
			LOG_CRITICAL("Error creating window: {}", SDL_GetError());
			return {};
		}

		for (int i = 0, len = SDL_GetNumRenderDrivers(); i != len; ++i)
		{
			if (SDL_RendererInfo info{};
				SDL_GetRenderDriverInfo(i, &info) == 0)
			{
				LOG_INFO("SDL renderer driver #{}: {}", i, info.name);
			}
		}

		auto renderer = SdlRenderer(SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_ACCELERATED));

		if (!renderer)
		{
			LOG_CRITICAL("Error creating renderer: {}", SDL_GetError());
			return {};
		}

		if (SDL_RendererInfo info{};
			SDL_GetRendererInfo(renderer.get(), &info) == 0)
		{
			LOG_INFO("Using renderer driver: {}", info.name);
		}

		if (SDL_RenderSetLogicalSize(renderer.get(), logical_size.w, logical_size.h) != 0)
			LOG_WARN("Error setting renderer logical size: {}", SDL_GetError());

		auto renderer_viewport = viewport(renderer.get());

		return App{std::make_unique<Core>(
			std::move(sdl),
			std::move(window_title),
			window_size,
			logical_size,
			renderer_viewport,
			std::move(window),
			std::move(renderer))};
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

						// update our window size to reflect the change
						SDL_GetWindowSize(core->window.get(), &core->window_size.w, &core->window_size.h);

						core->renderer_viewport = viewport(core->renderer.get());
						LOG_INFO("Renderer viewport is {},{} {}x{}", core->renderer_viewport.x, core->renderer_viewport.y, core->renderer_viewport.w, core->renderer_viewport.h);
					}
					break;

				case SDL_DROPFILE:
					if (on_file_drop)
						on_file_drop(*this, event.drop.file);

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

		Clock clock;
		FPS fps;

		while (process_events())
		{
			// tick
			auto real_deltatime = clock.update().count();
			fps.update(real_deltatime);

			// if we have a large frameskip, cap it
			auto deltatime = std::min(real_deltatime, 0.25);

			if (fps.accumulated_time() >= 1.0)
			{
				SDL_SetWindowTitle(core->window.get(), fmt::format("{0} - FPS: {1:.2f}     Best: {2:.2f}ms  Worst: {3:.2f}ms  Median: {4:.2f}ms  Average: {5:.2f}ms", core->window_title, fps.fps(), fps.best(), fps.worst(), fps.median(), fps.average()).c_str());
				fps.reset();
			}

			core->input.update(*core);
			if (on_update)
			{
				auto r = Renderer(core->renderer.get());
				on_update(*this, r, deltatime);
			}

			// render
			SDL_RenderPresent(core->renderer.get());
		}
	}

	void App::fullscreen(bool mode) noexcept
	{
		int flags = 0;
		if (mode)
			flags = SDL_WINDOW_FULLSCREEN_DESKTOP;

		if (SDL_SetWindowFullscreen(core->window.get(), flags) != 0)
			LOG_WARN("Could not set fullscreen to {}, reason: {}", mode, SDL_GetError());
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

	bool App::modifiers(KeyMods mods) const noexcept
	{
		return (core->input.modifiers() & mods) != KeyMods::none;
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

	cm::Sizei App::renderer_size() const noexcept
	{
		cm::Sizei size;
		SDL_RenderGetLogicalSize(core->renderer.get(), &size.w, &size.h);

		return size;
	}

	void App::enable_screensaver(bool enable) noexcept
	{
		if (enable)
		{
			LOG_INFO("Screensaver enabled");
			SDL_EnableScreenSaver();
		}
		else
		{
			LOG_INFO("Screensaver disabled");
			SDL_DisableScreenSaver();
		}
	}

	AudioDevice App::create_audio_device(int frequency, int channels, int sample_size) const noexcept
	{
		SDL_AudioSpec device_spec;

		for (int i = 0, num_devices = SDL_GetNumAudioDevices(false);
			 i < num_devices; ++i)
		{
			auto name = SDL_GetAudioDeviceName(i, false);
			SDL_GetAudioDeviceSpec(i, false, &device_spec);

			LOG_INFO("Audio device #{}: '{}' {} Hz, {} channels, {}-bit {}", i, name, device_spec.freq, device_spec.channels, SDL_AUDIO_BITSIZE(device_spec.format),
				SDL_AUDIO_ISFLOAT(device_spec.format)        ? "float"
					: SDL_AUDIO_ISSIGNED(device_spec.format) ? "signed int"
															 : "unsigned int");
		}

		return AudioDevice::create(frequency, channels, sample_size);
	}

	Texture App::create_texture(cm::Sizei size) noexcept
	{
		auto texture = SDL_CreateTexture(core->renderer.get(), SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, size.w, size.h);
		if (!texture)
		{
			LOG_WARN("Error creating texture: {}", SDL_GetError());
			return {};
		}

		// this should never fail. The docs indicate that this will fail if the texture is invalid, but we just created a valid texture.
		if (SDL_SetTextureScaleMode(texture, SDL_ScaleModeNearest) != 0) [[unlikely]]
			LOG_WARN("Error setting texture scale mode: {}", SDL_GetError());

		return Texture(texture);
	}

	std::filesystem::path App::get_application_path() noexcept
	{
		auto path = SDL_GetBasePath();
		if (!path)
			return "";

		return path;
	}

	std::filesystem::path App::get_user_data_path(const std::string &company_name, const std::string &app_name) noexcept
	{
		auto path = SDL_GetPrefPath(company_name.c_str(), app_name.c_str());
		if (!path)
			return "";

		return path;
	}

	std::filesystem::path App::get_user_data_path(const std::string &app_name) noexcept
	{
		return get_user_data_path("", app_name);
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

	void InputState::update(const App::Core &app) noexcept
	{
		int num_keys;
		auto keys = SDL_GetKeyboardState(&num_keys);

		// swap and assign should avoid having to reallocate every frame and encourage memory reuse
		std::swap(last_keys, current_keys);
		current_keys.assign(keys, keys + num_keys);

		mods = static_cast<KeyMods>(SDL_GetModState());

		// update mouse position and buttons
		last_mouse_buttons = std::exchange(current_mouse_buttons, SDL_GetMouseState(&mouse_pos.x, &mouse_pos.y));

		float fx, fy;
		SDL_RenderWindowToLogical(app.renderer.get(), mouse_pos.x, mouse_pos.y, &fx, &fy);

		mouse_pos.x = static_cast<int>(fx);
		mouse_pos.y = static_cast<int>(fy);
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
