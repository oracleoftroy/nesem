#pragma once

#include <filesystem>
#include <span>
#include <utility>
#include <vector>

#include <debugbreak.h>
#include <spdlog/spdlog.h>

#include <util/preprocessor.hpp>

#define LOG_LEVEL_TRACE 0
#define LOG_LEVEL_DEBUG 1
#define LOG_LEVEL_INFO 2
#define LOG_LEVEL_WARN 3
#define LOG_LEVEL_ERROR 4
#define LOG_LEVEL_CRITICAL 5
#define LOG_LEVEL_DISABLED 6

#if !defined(LOG_ACTIVE_LEVEL)
#	if defined(DEBUG)
#		define LOG_ACTIVE_LEVEL LOG_LEVEL_DEBUG
#	else
#		define LOG_ACTIVE_LEVEL LOG_LEVEL_INFO
#	endif
#endif

#if !defined(SPDLOG_ACTIVE_LEVEL)
#	define SPDLOG_ACTIVE_LEVEL LOG_ACTIVE_LEVEL
#endif

#include <spdlog/async.h>

#if defined(__EMSCRIPTEN__)
#	include <emscripten.h>

#	include <spdlog/details/null_mutex.h>
#	include <spdlog/sinks/base_sink.h>

namespace util::detail::sinks
{
	template <typename Mutex>
	class emscripten_sink final : public spdlog::sinks::base_sink<Mutex>
	{
		int flags;

	public:
		explicit emscripten_sink(int flags = EM_LOG_CONSOLE)
			: flags(flags)
		{
		}

	private:
		int get_flags(const spdlog::details::log_msg &msg)
		{
			switch (msg.level)
			{
			case spdlog::level::trace:
			case spdlog::level::debug:
				return flags | EM_LOG_DEBUG;

			case spdlog::level::info:
				return flags | EM_LOG_INFO;

			case spdlog::level::warn:
				return flags | EM_LOG_WARN;

			case spdlog::level::err:
			case spdlog::level::off:
				return flags | EM_LOG_ERROR;

			case spdlog::level::critical:
				return flags | EM_LOG_ERROR | EM_LOG_C_STACK | EM_LOG_JS_STACK;

			default:
				return flags | EM_LOG_INFO;
			}
		}

		void sink_it_(const spdlog::details::log_msg &msg) override
		{
			spdlog::memory_buf_t formatted;
			spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);

			emscripten_log(get_flags(msg), "%.*s", formatted.size(), formatted.data());
		}

		void flush_() override
		{
		}
	};

	using emscripten_sink_mt = emscripten_sink<std::mutex>;
	using emscripten_sink_st = emscripten_sink<spdlog::details::null_mutex>;
}

#else
#	include <spdlog/sinks/stdout_color_sinks.h>
#	include <spdlog/sinks/basic_file_sink.h>
#	if defined(_WIN32)
#		include <spdlog/sinks/msvc_sink.h>
#	endif
#endif

#define LOG(level, ...) SPDLOG_LOGGER_CALL(spdlog::default_logger_raw(), spdlog::level::level_enum(level), __VA_ARGS__)

#define LOG_TRACE SPDLOG_TRACE
#define LOG_DEBUG SPDLOG_DEBUG
#define LOG_INFO SPDLOG_INFO
#define LOG_WARN SPDLOG_WARN
#define LOG_ERROR SPDLOG_ERROR
#define LOG_CRITICAL SPDLOG_CRITICAL

#define LOG_ONCE(DO_LOG, ...)                            \
	do                                                   \
	{                                                    \
		static bool PP_UNIQUE_VAR(already_ran_) = false; \
		if (PP_UNIQUE_VAR(already_ran_))                 \
			break;                                       \
		PP_UNIQUE_VAR(already_ran_) = true;              \
		DO_LOG(__VA_ARGS__);                             \
	} while (false)

#define LOG_TRACE_ONCE(...) LOG_ONCE(LOG_TRACE, __VA_ARGS__)
#define LOG_DEBUG_ONCE(...) LOG_ONCE(LOG_DEBUG, __VA_ARGS__)
#define LOG_INFO_ONCE(...) LOG_ONCE(LOG_INFO, __VA_ARGS__)
#define LOG_WARN_ONCE(...) LOG_ONCE(LOG_WARN, __VA_ARGS__)
#define LOG_ERROR_ONCE(...) LOG_ONCE(LOG_ERROR, __VA_ARGS__)
#define LOG_CRITICAL_ONCE(...) LOG_ONCE(LOG_CRITICAL, __VA_ARGS__)

#if !defined(DEBUG)
#	define VERIFY(condition, reason) condition
#	define CHECK(condition, reason)
#	define DEBUG_BREAK()

#else
#	define DEBUG_BREAK() debug_break()

#	define CHECK(condition, reason)                                   \
		if (!(condition)) [[unlikely]]                                 \
		{                                                              \
			LOG_CRITICAL("Check '{}' failed: {}", #condition, reason); \
			DEBUG_BREAK();                                             \
		}

#	define VERIFY(condition, reason)                                    \
		[&](bool debug_check_condition) {                                \
			if (!debug_check_condition) [[unlikely]]                     \
			{                                                            \
				LOG_ERROR("Verify '{}' failed: {}", #condition, reason); \
				DEBUG_BREAK();                                           \
			}                                                            \
			return debug_check_condition;                                \
		}(condition)

#endif

#if defined(_WIN32) && !defined(_WINDOWS_)
// #	define NOMINMAX
// #	include <Windows.h>
// #	undef far
// #	undef near

// avoid pulling in the monstrosity known as Windows.h in a public header
constexpr auto ATTACH_PARENT_PROCESS = uint32_t(-1);
extern "C" __declspec(dllimport) int32_t __stdcall AttachConsole(uint32_t dwProcessId);
// extern "C" __declspec(dllimport) uint32_t __stdcall GetLastError();

#endif

namespace util::detail
{
	struct LoggerInit
	{
		// using a shorter pattern for logging to consoles to fit more on screen
		// plus we don't need the date for something shown immediately
		static constexpr char console_pattern[] = "[%H:%M:%S.%e] [%^%l%$] [%s:%#] %v";

		// the pattern for files and other longer term sinks, currently using spdlog's default
		static constexpr char file_pattern[] = "%+";

		LoggerInit(const std::filesystem::path &filename = {})
		{
			std::vector<spdlog::sink_ptr> sinks;

			setup_console_sinks(sinks);
			setup_file_sinks(sinks, filename);

#if defined(__EMSCRIPTEN__)
			auto logger = std::make_shared<spdlog::logger>("", begin(sinks), end(sinks));
#else
			spdlog::init_thread_pool(8192, 1);
			auto logger = std::make_shared<spdlog::async_logger>("", begin(sinks), end(sinks), spdlog::thread_pool());
#endif

			spdlog::set_default_logger(std::move(logger));

			spdlog::set_level(spdlog::level::level_enum(SPDLOG_ACTIVE_LEVEL));

			std::set_terminate([] {
				spdlog::shutdown();
				std::abort();
			});
		}

		~LoggerInit()
		{
			spdlog::shutdown();
		}

		void setup_console_sinks(std::vector<spdlog::sink_ptr> &sinks)
		{
			auto configure_sink = [](auto &&sink) {
				sink->set_pattern(console_pattern);
				sink->set_level(spdlog::level::info);
				return sink;
			};

#if defined(__EMSCRIPTEN__)
			sinks.emplace_back(configure_sink(std::make_shared<sinks::emscripten_sink_mt>()));
#else
			sinks.emplace_back(configure_sink(std::make_shared<spdlog::sinks::stdout_color_sink_mt>()));

#	if defined(_WIN32)
			// try to attach to the parent console
			// If we were run from the commandline, this will allow normal console I/O to work, useful for logging
			// Failure is fine, assume we don't have a parent to attach to.
			// -1 == ATTACH_PARENT_PROCESS
			AttachConsole(ATTACH_PARENT_PROCESS);
			sinks.emplace_back(configure_sink(std::make_shared<spdlog::sinks::msvc_sink_mt>()));
#	endif
#endif
		}

		void setup_file_sinks([[maybe_unused]] std::vector<spdlog::sink_ptr> &sinks, [[maybe_unused]] const std::filesystem::path &filename)
		{
#if !defined(__EMSCRIPTEN__)
			if (filename.empty())
				return;

			auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filename.string(), true);
			file_sink->set_pattern(file_pattern);
			file_sink->set_level(spdlog::level::debug);

			sinks.push_back(std::move(file_sink));
#endif
		}
	};
}
