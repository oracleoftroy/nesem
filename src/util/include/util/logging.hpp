#pragma once

#include <filesystem>
#include <utility>

#include <debugbreak.h>
#include <spdlog/sinks/ansicolor_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/wincolor_sink.h>
#include <spdlog/spdlog.h>

#define LOG_TRACE SPDLOG_TRACE
#define LOG_DEBUG SPDLOG_DEBUG
#define LOG_INFO SPDLOG_INFO
#define LOG_WARN SPDLOG_WARN
#define LOG_ERROR SPDLOG_ERROR
#define LOG_CRITICAL SPDLOG_CRITICAL

#define PP_CONCAT1(X, Y) X##Y
#define PP_CONCAT(X, Y) PP_CONCAT1(X, Y)
#define PP_UNIQUE_VAR(name) PP_CONCAT(name, __LINE__)

#define LOG_ONCE(LOG, ...)                               \
	do                                                   \
	{                                                    \
		static bool PP_UNIQUE_VAR(already_ran_) = false; \
		if (PP_UNIQUE_VAR(already_ran_))                 \
			break;                                       \
		PP_UNIQUE_VAR(already_ran_) = true;              \
		LOG(__VA_ARGS__);                                \
	} while (false)

#define LOG_TRACE_ONCE(...) LOG_ONCE(LOG_TRACE, __VA_ARGS__)
#define LOG_DEBUG_ONCE(...) LOG_ONCE(LOG_DEBUG, __VA_ARGS__)
#define LOG_INFO_ONCE(...) LOG_ONCE(LOG_INFO, __VA_ARGS__)
#define LOG_WARN_ONCE(...) LOG_ONCE(LOG_WARN, __VA_ARGS__)
#define LOG_ERROR_ONCE(...) LOG_ONCE(LOG_ERROR, __VA_ARGS__)
#define LOG_CRITICAL_ONCE(...) LOG_ONCE(LOG_CRITICAL, __VA_ARGS__)

#if defined(NDEBUG)
#	define VERIFY(condition, reason) condition
#	define CHECK(condition, reason)
#	define DEBUG_BREAK()

#else
#	define DEBUG_BREAK() debug_break()

#	define CHECK(condition, reason)                                      \
		[&](bool debug_check_condition) {                                 \
			if (!debug_check_condition)                                   \
			{                                                             \
				SPDLOG_WARN("Check '{}' failed: {}", #condition, reason); \
				DEBUG_BREAK();                                            \
			}                                                             \
		}(condition)

#	define VERIFY(condition, reason)                                      \
		[&](bool debug_check_condition) {                                  \
			if (!debug_check_condition)                                    \
			{                                                              \
				SPDLOG_WARN("Verify '{}' failed: {}", #condition, reason); \
				DEBUG_BREAK();                                             \
			}                                                              \
			return debug_check_condition;                                  \
		}(condition)

#endif

#if defined(_WIN32)
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
		static constexpr char console_pattern[] = "[%H:%M:%S.%e] [%^%l%$] [%s:%#] %v";
		static constexpr char default_pattern[] = "%+";

		LoggerInit(const std::filesystem::path &filename = {})
		{
			auto logger = std::make_shared<spdlog::logger>("");

			setup_console_sinks(*logger);
			setup_file_sinks(*logger, filename);

			spdlog::set_default_logger(std::move(logger));
		}

		void setup_console_sinks(spdlog::logger &logger)
		{
#if defined(_WIN32)
			// try to attach to the parent console
			// If we were run from the commandline, this will allow normal console I/O to work, useful for logging
			// Failure is fine, assume we don't have a parent to attach to.
			// -1 == ATTACH_PARENT_PROCESS
			if (AttachConsole(ATTACH_PARENT_PROCESS))
				logger.sinks().emplace_back(std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>());

			logger.sinks().emplace_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());

#else
			logger.sinks().emplace_back(std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>());
#endif
			// spdlog creates a sink to console and we added one to a debugger
			// as these are both immediate current run sorts of things, provide a slightly shorter pattern,
			// basically the default without the current date
			for (auto &sink : logger.sinks())
				sink->set_pattern(console_pattern);
		}

		void setup_file_sinks(spdlog::logger &logger, const std::filesystem::path &filename)
		{
			if (filename.empty())
				return;

			auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filename.string(), true);
			file_sink->set_pattern(default_pattern);

			logger.sinks().push_back(std::move(file_sink));
		}
	};

	struct LoggerInitDummy
	{
		const static inline LoggerInit logger_init;
	};

}
