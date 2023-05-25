#pragma once

#include <functional>
#include <memory>
#include <version>

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

#if defined(__cpp_lib_move_only_function)
		using ConstructFn = std::move_only_function<MakeCartFn()>;
#else
		using ConstructFn = std::function<MakeCartFn()>;
#endif

		bool register_cart(int ines_mapper, ConstructFn fn);

		template <typename T>
		auto construct_helper()
		{
			return [](const Nes &nes, mappers::NesRom &&rom) {
				return std::make_unique<T>(nes, std::move(rom));
			};
		}
	}

#define REGISTER_MAPPER(ines, Type)                                         \
	static inline bool PP_UNIQUE_VAR(do_register_mapper_)()                 \
	{                                                                       \
		return detail::register_cart(ines, detail::construct_helper<Type>); \
	}                                                                       \
	inline static const auto PP_UNIQUE_VAR(cart_is_registered_detail_) = PP_UNIQUE_VAR(do_register_mapper_)()
}
