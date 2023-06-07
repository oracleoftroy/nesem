#include "nes_nvram.hpp"

#include <fstream>

#include <fmt/std.h>
#include <mio/mmap.hpp>

#include <util/logging.hpp>

namespace
{
	auto open_file(const std::filesystem::path &file_name, size_t size) noexcept
	{
		mio::ummap_sink result;

		std::error_code ec;

		// First, make sure the parent directory already exists
		auto created = std::filesystem::create_directories(file_name.parent_path(), ec);
		if (ec)
		{
			LOG_ERROR("Could not create directory: {}", file_name.parent_path());
			return result;
		}
		else if (created)
			LOG_DEBUG("Path created: {}", file_name.parent_path());
		else
			LOG_DEBUG("Path already existed: {}", file_name.parent_path());

		// mio requires that the file already exist of the proper size, so make a file if we need to
		if (!exists(file_name, ec))
		{
			LOG_INFO("NVRAM file does not exist, creating: {}", file_name);
			std::ofstream f(file_name, std::ios::binary);
		}

		if (auto current_size = file_size(file_name);
			current_size < size)
		{
			LOG_INFO("Current NVRAM size is {:L}, resizing to {:L}", current_size, size);
			resize_file(file_name, size, ec);
			if (ec)
			{
				LOG_ERROR("Could not resize NVRAM file: {}", file_name);
				return result;
			}
		}
		else if (current_size > size)
			LOG_WARN("Existing NVRAM file larger? Size is{:L} but only requesting {:L}", current_size, size);

		LOG_INFO("opening {:L}K of NVRAM at {}", size / 1024, file_name);

		result.map(file_name.string(), 0, size, ec);

		if (ec)
			LOG_ERROR("Could not open NVRAM at '{}', reason: {}", file_name, ec.message());

		return result;
	}
}

namespace nesem
{
	// this mostly exists so we can hide the mio includes from the rest of the world, as they include windows.h in a header only library
	// opening and closing files isn't on the hot path, so the extra indirection and allocation isn't a big deal, but we cache a pointer
	// (via a span) of the mapped data so that reading and writing won't go through the extra indirection
	struct NesNvram::Core
	{
		mio::ummap_sink file;
	};

	NesNvram::NesNvram(const std::filesystem::path &file_name, size_t size) noexcept
		: core(std::make_unique<Core>(open_file(file_name, size)))
	{
		if (core->file.is_open())
			data = core->file;
	}

	NesNvram::NesNvram() noexcept = default;
	NesNvram::~NesNvram() = default;
	NesNvram::NesNvram(NesNvram &&other) noexcept = default;
	NesNvram &NesNvram::operator=(NesNvram &&other) noexcept = default;
}
