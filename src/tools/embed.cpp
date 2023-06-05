#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <expected>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <ranges>
#include <regex>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <cryptopp/cryptlib.h>
#include <cryptopp/filters.h>
#include <cryptopp/gzip.h>
#include <cryptopp/zdeflate.h>
#include <cryptopp/zinflate.h>
#include <cryptopp/zlib.h>
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/std.h>
#include <mio/mmap.hpp>

#include <util/ptr.hpp>

namespace io
{
	using fmt::format;
	using fmt::print;
	using fmt::println;
}

enum class CompressType
{
	None,
	Deflate,
	Gzip,
	Zlib,
};

std::unique_ptr<CryptoPP::BufferedTransformation> make_compress_sink(CompressType type, std::vector<CryptoPP::byte> &dest)
{
	using namespace CryptoPP;

	auto sink = std::make_unique<VectorSink>(dest);

	switch (type)
	{
	case CompressType::None:
		return sink;
	case CompressType::Deflate:
		return std::make_unique<Deflator>(sink.release());
	case CompressType::Gzip:
		return std::make_unique<Gzip>(sink.release());
	case CompressType::Zlib:
		return std::make_unique<ZlibCompressor>(sink.release());
	}

	std::unreachable();
}

std::unique_ptr<CryptoPP::BufferedTransformation> make_decompress_sink(CompressType type, std::vector<CryptoPP::byte> &dest)
{
	using namespace CryptoPP;

	auto sink = std::make_unique<VectorSink>(dest);

	switch (type)
	{
	case CompressType::None:
		return sink;
	case CompressType::Deflate:
		return std::make_unique<Inflator>(sink.release());
	case CompressType::Gzip:
		return std::make_unique<Gunzip>(sink.release());
	case CompressType::Zlib:
		return std::make_unique<ZlibDecompressor>(sink.release());
	}

	std::unreachable();
}

std::string get_header_text(CompressType type)
{
	switch (type)
	{
	case CompressType::None:
		return "";
	case CompressType::Deflate:
		return "#include <span>\n"
			   "#include <cryptopp/zinflate.h>";
	case CompressType::Gzip:
		return "#include <span>\n"
			   "#include <cryptopp/gzip.h>";
	case CompressType::Zlib:
		return "#include <span>\n"
			   "#include <cryptopp/zlib.h>";
	}

	std::unreachable();
}

std::string get_decompressor_text(CompressType type, size_t uncompressed_size)
{
	constexpr auto fn = R"(std::vector<unsigned char> decompress(std::span<const unsigned char> data)
	{{
		using namespace CryptoPP;

		constexpr size_t uncompressed_size = {1};
		std::vector<unsigned char> result;
		result.reserve(uncompressed_size);

		auto sink = std::make_unique<VectorSink>(result);
		auto decompressor = std::make_unique<{0}>(sink.release());
		StringSource source(data.data(), data.size(), true, decompressor.release());

		CHECK(result.size() == uncompressed_size, "Actual decompressed size does not match expected value");

		return result;
	}})";

	switch (type)
	{
	case CompressType::None:
		return "";
	case CompressType::Deflate:
		return io::format(fn, "Inflator", uncompressed_size);
	case CompressType::Gzip:
		return io::format(fn, "Gunzip", uncompressed_size);
	case CompressType::Zlib:
		return io::format(fn, "ZlibDecompressor", uncompressed_size);
	}

	std::unreachable();
}

std::string call_decompressor_text(CompressType type, std::string_view data_name)
{
	switch (type)
	{
	case CompressType::None:
		return io::format("return std::vector({0}.begin(), {0}.end());", data_name);
	case CompressType::Deflate:
	case CompressType::Gzip:
	case CompressType::Zlib:
		return io::format("return decompress({0});", data_name);
	}

	std::unreachable();
}

std::vector<CryptoPP::byte> compress(CompressType type, std::span<const unsigned char> data)
{
	std::vector<CryptoPP::byte> result;
	CryptoPP::StringSource source(data.data(), data.size(), true, make_compress_sink(type, result).release());

	return result;
}

std::vector<CryptoPP::byte> decompress(CompressType type, std::span<const unsigned char> data)
{
	std::vector<CryptoPP::byte> result;
	CryptoPP::StringSource source(data.data(), data.size(), true, make_decompress_sink(type, result).release());

	return result;
}

std::expected<CompressType, std::string> parse_compression(std::string arg)
{
	using enum CompressType;

	static const auto regex_none = std::regex("none", std::regex::icase);
	static const auto regex_deflate = std::regex("deflate", std::regex::icase);
	static const auto regex_gzip = std::regex("gzip", std::regex::icase);
	static const auto regex_zlib = std::regex("zlib", std::regex::icase);

	if (std::regex_match(arg, regex_none))
		return None;
	if (std::regex_match(arg, regex_deflate))
		return Deflate;
	if (std::regex_match(arg, regex_gzip))
		return Gzip;
	if (std::regex_match(arg, regex_zlib))
		return Zlib;

	return std::unexpected(io::format("invalid compression type '{}'", arg));
}

std::string to_symbol_name(const std::string &name)
{
	static const auto invalid_regex = std::regex(R"(\W)");
	return std::regex_replace(name, invalid_regex, "_");
}

std::string_view to_string(CompressType type)
{
	using std::operator""sv;
	using enum CompressType;

	switch (type)
	{
	case None:
		return "none"sv;
	case Deflate:
		return "deflate"sv;
	case Gzip:
		return "gzip"sv;
	case Zlib:
		return "zlib"sv;
	}

	std::unreachable();
}

struct Options
{
	std::string exe{};
	std::filesystem::path input_filename{};
	std::optional<std::string> output_file_name{};
	std::optional<std::string> symbol_name{};
	std::optional<std::string> namespace_name{};
	CompressType compression{};
	bool show_help{};
	bool test_mode{};
};

void print_options(const Options &options)
{
	io::println("exe: {}", options.exe);
	io::println("input_filename: {}", options.input_filename.string());
	io::println("output_file_name: {}", options.output_file_name);
	io::println("symbol_name: {}", options.symbol_name);
	io::println("compression: {}", to_string(options.compression));
	io::println("show_help: {}", options.show_help);
}

std::expected<Options, std::string> parse_command_line(std::span<char *> args)
{
	auto it = args.begin();
	const auto end = args.end();

	Options result;

	// skip the first argument (should be the program name)
	while (++it != end)
	{
		auto arg = std::string_view{*it};

		if (!arg.starts_with('-'))
		{
			if (result.input_filename.empty())
				result.input_filename = arg;
			else
				return std::unexpected(io::format("input file '{}', but was already set to '{}'", arg, result.input_filename.string()));
		}
		else if (arg == "--help" || arg == "-h" || arg == "-?")
		{
			result.show_help = true;
		}
		else if (arg == "--out" || arg == "-o")
		{
			if (++it != end)
				result.output_file_name = *it;
			else
				return std::unexpected(io::format("'{}' specified, but no argument given", arg));
		}
		else if (arg == "--symbol" || arg == "-s")
		{
			if (++it != end)
				result.symbol_name = *it;
			else
				return std::unexpected(io::format("'{}' specified, but no argument given", arg));
		}
		else if (arg == "--compress" || arg == "-c")
		{
			if (++it != end)
			{
				if (auto compression = parse_compression(*it);
					compression.has_value())
					result.compression = *compression;
				else
					return std::unexpected(compression.error());
			}
			else
				return std::unexpected(io::format("'{}' specified, but no argument given", arg));
		}
		else if (arg == "--namespace" || arg == "-n")
		{
			if (++it != end)
				result.namespace_name = *it;
			else
				return std::unexpected(io::format("'{}' specified, but no argument given", arg));
		}
		else if (arg == "--test")
		{
			result.test_mode = true;
		}
	}

	return result;
}

void print_help(std::string_view app)
{
	io::println("USAGE: {} [ops] <input filename>", app);
	io::println("OPTIONS:");
	io::println("--help,-h,-?          - print this help");
	io::println("--out,-o       <name> - generates source and header of name: <name>.{{cpp,hpp}}, default to name of file");
	io::println("--symbol,-s    <name> - name of getter function, defaults to <input filename> (invalid characters converted to '_')");
	io::println("--compress,-c  <type> - compress using algorithm <type>: one of none, deflate, gzip, zlib, default none");
	io::println("--namespace,-n <name> - wrap function in namespace <name>");
	io::println("--test                - test input file against each compression type and print results");
}

void print_error(std::string_view msg)
{
	io::println(stderr, "{}", msg);
}

[[noreturn]] void on_error(const Options &options, std::string_view msg)
{
	print_error(msg);
	print_help(options.exe);
	std::exit(EXIT_FAILURE);
}

std::filesystem::path outfile_path(const Options &options, std::string_view ext)
{
	return std::filesystem::path{options.output_file_name.value_or(options.input_filename.filename().string())}.concat(ext);
}

std::string symbol_name(const Options &options)
{
	return to_symbol_name(options.symbol_name.value_or(options.input_filename.filename().string()));
}

void write(const Options &options, size_t uncompressed_size, std::span<unsigned char> data)
{
	auto header_path = outfile_path(options, ".hpp");
	auto source_path = outfile_path(options, ".cpp");

	auto fn_name = symbol_name(options);
	std::string ns_start;
	std::string ns_end;

	if (options.namespace_name)
	{
		ns_start = io::format("namespace {} {{\n", *options.namespace_name);
		ns_end = "}";
	}

	{
		auto header_file = std::ofstream{header_path};
		if (!header_file)
			on_error(options, io::format("error opening {} for write", header_path));

		io::print(header_file,
			R"(#pragma once

#include <vector>

{1}std::vector<unsigned char> {0}();
{2})",
			fn_name, ns_start, ns_end);
	}

	{
		auto source_file = std::ofstream{source_path};
		if (!source_file)
			on_error(options, io::format("error opening {} for write", source_path));

		io::print(source_file,
			R"(#include "{0}"
#include <array>
#include <memory>
{3}
#include <util/logging.hpp>

namespace
{{
	static constexpr std::array<unsigned char, {1}> {2}_data = {{
		// clang-format off
)",
			header_path.string(), data.size(), fn_name, get_header_text(options.compression));

		for (auto &&batch : std::views::chunk(data, 16))
		{
			io::println(source_file, "\t\t{:#04x},", fmt::join(batch, ", "));
		}

		auto data_name = io::format("{0}_data", fn_name);

		io::println(source_file,
			R"(	// clang-format on
	}};

	{3}
}}

{1}std::vector<unsigned char> {0}()
{{
	{4}
}}
{2})",
			fn_name, ns_start, ns_end, get_decompressor_text(options.compression, uncompressed_size), call_decompressor_text(options.compression, data_name));
	}
}

void run_tests(const mio::ummap_source &file)
{
	const size_t original_size = file.size();

	std::atomic<std::chrono::steady_clock::time_point> start;
	std::atomic<std::chrono::steady_clock::time_point> end;

	for (auto type : {CompressType::Zlib, CompressType::Gzip, CompressType::Deflate})
	{
		start.store(std::chrono::steady_clock::now());
		auto compress_result = compress(type, file);
		end.store(std::chrono::steady_clock::now());

		auto compress_time = end.load(std::memory_order::relaxed) - start.load(std::memory_order::relaxed);

		start.store(std::chrono::steady_clock::now());
		auto decompress_result = decompress(type, compress_result);
		end.store(std::chrono::steady_clock::now());

		auto decompress_time = end.load(std::memory_order::relaxed) - start.load(std::memory_order::relaxed);

		auto size_compress = compress_result.size();
		auto compress_percent = 100.0 * static_cast<double>(original_size - size_compress) / static_cast<double>(original_size);

		io::println("{:>7}: compress by {:0.4}% - compress time: {} - decompress time: {}",
			to_string(type), compress_percent, duration_cast<std::chrono::milliseconds>(compress_time), duration_cast<std::chrono::milliseconds>(decompress_time));

		if (original_size != decompress_result.size())
			io::println("PROBLEM WITH ALGORIGM!!! FIX IT NAOW!!!! {} != {}", original_size, decompress_result.size());
	}
}

void run(const Options &options)
{
	if (options.show_help)
	{
		print_help(options.exe);
		return;
	}

	if (options.input_filename.empty())
		on_error(options, "No filename specified");

	std::error_code err;
	mio::ummap_source file;
	file.map(options.input_filename.native(), err);

	if (err)
		on_error(options, err.message());

	if (options.test_mode)
	{
		run_tests(file);
		return;
	}

	auto compressed = compress(options.compression, file);
	auto size_original = file.size();
	auto size_compress = compressed.size();
	auto compress_percent = 100.0 * static_cast<double>(size_original - size_compress) / static_cast<double>(size_original);
	io::println("original size: {}", size_original);
	io::println("compressed size: {}", size_compress);
	io::println("compression: {}%", compress_percent);

	write(options, size_original, compressed);
}

int main(int argc, char *argv[])
{
	if (argc == 0)
	{
		// This "should" never happen. Technically possible, but windows and unix-likes always provide at least 1 argument
		print_error("Commandline empty?!?!");
		return EXIT_FAILURE;
	}

	auto exe = std::filesystem::path(argv[0]).filename().string();

	auto options = parse_command_line({argv, argv + argc});

	if (options.has_value())
	{
		options->exe = exe;
		run(*options);
	}
	else
	{
		io::println(stderr, "{}", options.error());
		print_help(exe);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
