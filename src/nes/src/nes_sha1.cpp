#include "nes_sha1.hpp"

#include <array>

// Clang is unhappy about the way forward_range and input_range are applied to chunk_view for
// some reason, so directly include the nonstandard header for contiguous_range, if available
#if defined(__clang__) && __clang_major__ <= 16 && __has_include(<bits/ranges_base.h>)
#	include <bits/ranges_base.h>
#else
#	include <ranges>
#endif

#include <cryptopp/hex.h>
#include <cryptopp/sha.h>

namespace nesem::util
{
	std::string sha1_impl(std::ranges::contiguous_range auto... bytes) noexcept
	{
		std::array<CryptoPP::byte, CryptoPP::SHA1::DIGESTSIZE> digest{};

		CryptoPP::SHA1 sha;
		(sha.Update(data(bytes), size(bytes)), ...);
		sha.Final(data(digest));

		std::string output;

		auto encoder = CryptoPP::HexEncoder(new CryptoPP::StringSink(output));
		encoder.Put(data(digest), size(digest));
		encoder.MessageEnd();

		return output;
	}

	std::string sha1(std::span<const U8> data) noexcept
	{
		return sha1_impl(data);
	}

	std::string sha1(std::span<const U8> prgrom, std::span<const U8> chrrom) noexcept
	{
		return sha1_impl(prgrom, chrrom);
	}
}
