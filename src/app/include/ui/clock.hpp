#pragma once

#include <chrono>
#include <utility>

namespace ui
{
	struct Clock
	{
		using clock = std::chrono::steady_clock;
		clock::time_point current_time = clock::now();

		std::chrono::duration<double> update() noexcept
		{
			auto last_time = std::exchange(current_time, clock::now());
			return current_time - last_time;
		}
	};
}
