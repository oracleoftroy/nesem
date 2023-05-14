#pragma once

#include <functional>
#include <memory>

#include <util/preprocessor.hpp>

namespace nesem
{
	class Nes;
	class NesCartridge;

	namespace mappers
	{
		struct NesRom;
	}

	std::unique_ptr<NesCartridge> load_cartridge(const Nes &nes, mappers::NesRom rom) noexcept;

	namespace detail
	{
		using MakeCartFn = std::function<std::unique_ptr<NesCartridge>(const Nes &nes, mappers::NesRom &&rom)>;
		bool register_cart(int ines_mapper, MakeCartFn &&fn);

		template <typename T>
		auto construct_helper()
		{
			return [](const Nes &nes, mappers::NesRom &&rom) {
				return std::make_unique<T>(nes, std::move(rom));
			};
		}
	}

#define REGISTER_MAPPER(ines, Type) inline static const auto PP_UNIQUE_VAR(cart_is_registered_detail_) = detail::register_cart(ines, detail::construct_helper<Type>())
}
