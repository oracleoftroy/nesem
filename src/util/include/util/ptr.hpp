#pragma once

namespace util
{
	template <auto fn>
	struct deleter_from_fn
	{
		template <typename T>
		constexpr void operator()(T *arg) const noexcept
		{
			fn(arg);
		}
	};

	template <typename T, auto fn>
	using custom_unique_ptr = std::unique_ptr<T, deleter_from_fn<fn>>;
}
