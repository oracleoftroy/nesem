#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include <cm/math.hpp>

#include <ui/canvas.hpp>
#include <util/enum.hpp>

// hijack the main function a la SDL_main
#define main application_main

namespace ui
{
	class Key;
	class Scancode;
	enum class KeyMods;

	class App
	{
	public:
		static App create(const std::string &window_title, cm::Sizei window_size, cm::Sizei pixel_size = {1, 1}) noexcept;

		App() noexcept;
		~App();

		// callback function, called each tick
		std::function<void(App &app, Canvas &canvas, float deltatime)> on_update;

		// callback for handling dropped file
		std::function<void(std::string_view filename)> on_file_drop;

		// run the application. Exits when the window is closed
		void run();

		// verify that App is in a valid state
		explicit operator bool() noexcept;

		// Window functions

		// enter or leave fullscreen mode. This currently matches the current desktop resolution rather than change the monitor resolution to match the window
		void fullscreen(bool mode) noexcept;

		// The dimensions of the canvas
		[[nodiscard]] cm::Sizei canvas_size() const noexcept;

		// Input handling

		// look up a key by name. Slow, so call once and save the value to re-use a key
		[[nodiscard]] Key key_from_name(const char *name) noexcept;

		// look up a scancode by name. Slow, so call once and save the value to re-use a key
		[[nodiscard]] Scancode scancode_from_name(const char *name) noexcept;

		// get the current modifier keys (shift, alt, etc)
		[[nodiscard]] KeyMods modifiers() const noexcept;

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

	private:
		struct Core;
		std::unique_ptr<Core> core;

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
	MAKE_FLAGS_ENUM(KeyMods)

	class InputState;

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
