#pragma once

#include <span>
#include <string>

#include <nes_types.hpp>

namespace nesem::util
{
	std::string sha1(std::span<const U8> data) noexcept;
	std::string sha1(std::span<const U8> prgrom, std::span<const U8> chrrom) noexcept;
}
