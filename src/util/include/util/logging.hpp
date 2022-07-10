#pragma once

#include <filesystem>
#include <span>
#include <utility>
#include <vector>

#include <debugbreak.h>

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
#include <spdlog/sinks/ansicolor_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/wincolor_sink.h>
#include <spdlog/spdlog.h>

#define LOG(level, ...) SPDLOG_LOGGER_CALL(spdlog::default_logger_raw(), spdlog::level::level_enum(level), __VA_ARGS__)

#define LOG_TRACE SPDLOG_TRACE
#define LOG_DEBUG SPDLOG_DEBUG
#define LOG_INFO SPDLOG_INFO
#define LOG_WARN SPDLOG_WARN
#define LOG_ERROR SPDLOG_ERROR
#define LOG_CRITICAL SPDLOG_CRITICAL

#define PP_CONCAT1(X, Y) X##Y
#define PP_CONCAT(X, Y) PP_CONCAT1(X, Y)
#define PP_UNIQUE_VAR(name) PP_CONCAT(name, __LINE__)

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
			spdlog::init_thread_pool(8192, 1);
			std::vector<spdlog::sink_ptr> sinks;

			setup_console_sinks(sinks);
			setup_file_sinks(sinks, filename);

			auto logger = std::make_shared<spdlog::async_logger>("", begin(sinks), end(sinks), spdlog::thread_pool());
			// auto logger = std::make_shared<spdlog::logger>("", begin(sinks), end(sinks));
			spdlog::set_default_logger(std::move(logger));

			spdlog::set_level(spdlog::level::level_enum(SPDLOG_ACTIVE_LEVEL));
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

#if defined(_WIN32)
			// try to attach to the parent console
			// If we were run from the commandline, this will allow normal console I/O to work, useful for logging
			// Failure is fine, assume we don't have a parent to attach to.
			// -1 == ATTACH_PARENT_PROCESS
			if (AttachConsole(ATTACH_PARENT_PROCESS))
				sinks.emplace_back(configure_sink(std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>()));

			sinks.emplace_back(configure_sink(std::make_shared<spdlog::sinks::msvc_sink_mt>()));

#else
			sinks.emplace_back(configure_sink(std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>()));
#endif
		}

		void setup_file_sinks(std::vector<spdlog::sink_ptr> &sinks, const std::filesystem::path &filename)
		{
			if (filename.empty())
				return;

			auto configure_sink = [](auto &&sink) {
				return sink;
			};

			auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filename.string(), true);
			file_sink->set_pattern(file_pattern);
			file_sink->set_level(spdlog::level::debug);

			sinks.push_back(std::move(file_sink));
		}
	};
}
