#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <version>

#include <cm/math.hpp>
#include <ui/canvas.hpp>
#include <util/flags.hpp>

namespace ui
{
	class Key;
	class Scancode;
	enum class KeyMods;
	class AudioDevice;
	class Texture;
	class Renderer;
	class InputState;

	class App final
	{
	public:
		static App create(const std::string &window_title, cm::Sizei window_size, cm::Sizei pixel_size = {1, 1}) noexcept;

		App() noexcept;
		~App();

#if defined(__cpp_lib_move_only_function)
		// callback function, called each tick
		// I don't think reference_wrapper should be needed, but GCC 12 doesn't like incomplete types.
		// TODO: remove when GCC is fixed or delete these comments if not a library bug
		std::move_only_function<void(std::reference_wrapper<App> app, std::reference_wrapper<Renderer> renderer, double deltatime)> on_update;

		// callback for handling dropped file
		std::move_only_function<void(std::reference_wrapper<App> app, std::string_view filename)> on_file_drop;
#else
		// callback function, called each tick
		std::function<void(App &app, Renderer &renderer, double deltatime)> on_update;

		// callback for handling dropped file
		std::function<void(App &app, std::string_view filename)> on_file_drop;
#endif

		// run the application. Exits when the window is closed
		void run();

		// run one tick of the application, returning true if it should keep running, false if quit was requested
		bool run_once();

		// verify that App is in a valid state
		explicit operator bool() noexcept;

		// Window functions

		// enter or leave fullscreen mode. This currently matches the current desktop resolution rather than change the monitor resolution to match the window
		void fullscreen(bool mode) noexcept;

		// The dimensions of the renderer
		[[nodiscard]] cm::Sizei renderer_size() const noexcept;

		// SDL by default disables the screensaver, allow apps to modify this (for example, while paused)
		// if the screensaver is disabled, it will be re-enabled when the app exits
		void enable_screensaver(bool enable) noexcept;

		// Input handling

		// look up a key by name. Slow, so call once and save the value to re-use a key
		[[nodiscard]] static Key key_from_name(const char *name) noexcept;
		[[nodiscard]] static std::string_view name_from_key(Key key) noexcept;

		// look up a scancode by name. Slow, so call once and save the value to re-use a key
		[[nodiscard]] static Scancode scancode_from_name(const char *name) noexcept;
		[[nodiscard]] static std::string_view name_from_scancode(Scancode scancode) noexcept;

		// get the current modifier keys (shift, alt, etc)
		// TODO: consider deprecating...
		[[nodiscard]] util::Flags<KeyMods> modifiers() const noexcept;

		// test if any of the selected modifiers are active
		// caution, modifiers(alt | ctrl) triggers if either key is down, use modifiers(alt) && modifiers(ctrl) if you need both
		[[nodiscard]] bool modifiers(util::Flags<KeyMods> mods) const noexcept;

		// returns true if the key is down regardless of previous state
		[[nodiscard]] bool key_down(Key key) const noexcept;
		[[nodiscard]] bool key_down(Scancode code) const noexcept;

		// returns true if the key is up regardless of previous state
		[[nodiscard]] bool key_up(Key key) const noexcept;
		[[nodiscard]] bool key_up(Scancode code) const noexcept;

		// returns true if the key was up last frame and is now down, else false
		[[nodiscard]] bool key_pressed(Key key) const noexcept;
		[[nodiscard]] bool key_pressed(Scancode code) const noexcept;

		// returns true if the key was down last frame and is now up, else false
		[[nodiscard]] bool key_released(Key key) const noexcept;
		[[nodiscard]] bool key_released(Scancode code) const noexcept;

		// returns true if the button is down regardless of previous state
		[[nodiscard]] bool mouse_down(int button) const noexcept;

		// returns true if the button is up regardless of previous state
		[[nodiscard]] bool mouse_up(int button) const noexcept;

		// returns true if the button was up last frame and is now down, else false
		[[nodiscard]] bool mouse_pressed(int button) const noexcept;

		// returns true if the button was down last frame and is now up, else false
		[[nodiscard]] bool mouse_released(int button) const noexcept;

		// returns the mouse position scaled to the canvas size
		[[nodiscard]] cm::Point2i mouse_position() const noexcept;

		// Audio
		[[nodiscard]] AudioDevice create_audio_device(int frequency = 44100, int channels = 1, int sample_size = 512) const noexcept;

		// Texturing
		[[nodiscard]] Texture create_texture(cm::Sizei size) noexcept;

		// get directory this program was run from
		[[nodiscard]] static std::filesystem::path get_application_path() noexcept;

		// get directory for saving/loading user data (e.g. save files, etc)
		[[nodiscard]] static std::filesystem::path get_user_data_path(const std::string &company_name, const std::string &app_name) noexcept;
		[[nodiscard]] static std::filesystem::path get_user_data_path(const std::string &app_name) noexcept;

	private:
		struct Core;
		std::unique_ptr<Core> core;

		friend class InputState;

		explicit App(std::unique_ptr<Core> &&core) noexcept;
	};

	enum class KeyMods
	{
		none = 0x0000,
		left_shift = 0x0001,
		right_shift = 0x0002,
		left_ctrl = 0x0040,
		right_ctrl = 0x0080,
		left_alt = 0x0100,
		right_alt = 0x0200,
		left_gui = 0x0400,
		right_gui = 0x0800,

		numlock = 0x1000,
		capslock = 0x2000,
		alt_gr = 0x4000,

		ctrl = left_ctrl | right_ctrl,
		shift = left_shift | right_shift,
		alt = left_alt | right_alt,
		gui = left_gui | right_gui
	};

	class Key
	{
	public:
		constexpr Key() noexcept = default;

		constexpr auto operator<=>(const Key &other) const noexcept = default;
		constexpr explicit operator int() const noexcept;
		constexpr explicit operator bool() const noexcept;

	private:
		constexpr explicit Key(int value) noexcept;

		friend class App;
		friend class InputState;
		int value = 0;
	};

	class Scancode
	{
	public:
		constexpr Scancode() noexcept = default;

		constexpr auto operator<=>(const Scancode &other) const noexcept = default;
		constexpr explicit operator int() const noexcept;
		constexpr explicit operator bool() const noexcept;

	private:
		constexpr explicit Scancode(int value) noexcept;

		friend class App;
		friend class InputState;
		int value = 0;
	};

	constexpr Key::Key(int value) noexcept
		: value(value)
	{
	}

	constexpr Scancode::Scancode(int value) noexcept
		: value(value)
	{
	}

	constexpr Key::operator int() const noexcept
	{
		return value;
	}

	constexpr Key::operator bool() const noexcept
	{
		return value != 0;
	}

	constexpr Scancode::operator int() const noexcept
	{
		return value;
	}

	constexpr Scancode::operator bool() const noexcept
	{
		return value != 0;
	}
}
