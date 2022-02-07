#pragma once

#include <random>

namespace util
{
	template <typename RandomEngineType>
	inline RandomEngineType make_rng(std::seed_seq &seed) noexcept
	{
		return RandomEngineType(seed);
	}

	template <typename RandomEngineType>
	inline RandomEngineType make_rng() noexcept
	{
		auto rd = std::random_device{};
		auto seed = std::seed_seq{rd(), rd(), rd(), rd()};

		return make_rng<RandomEngineType>(seed);
	}

	class Random
	{
	public:
		using rng_engine = std::mt19937;

		template <std::integral T>
		T random_int(T min, T max)
		{
			auto dist = std::uniform_int_distribution<T>(min, max);
			return dist(rng);
		}

		template <std::integral T>
		T random_int(T max)
		{
			return random_int(T(0), max);
		}

		template <std::floating_point T>
		T random_float(T min, T max)
		{
			auto dist = std::uniform_real_distribution<T>(min, max);
			return dist(rng);
		}

		template <std::floating_point T>
		T random_float(T max)
		{
			return random_float(T(0), max);
		}

	private:
		rng_engine rng = make_rng<rng_engine>();
	};
}
