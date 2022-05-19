#pragma once

#include <memory>
#include <span>

namespace ui
{
	class AudioDevice final
	{
	public:
		static AudioDevice create(int frequency, int channels, int sample_size = 512) noexcept;

		explicit operator bool() noexcept;

		void pause(bool value) noexcept;

		void queue_audio(std::span<float> samples) noexcept;

	private:
		class AudioCore;
		std::unique_ptr<AudioCore> core;

		explicit AudioDevice(std::unique_ptr<AudioCore> &&core) noexcept;

	public:
		AudioDevice() noexcept;
		~AudioDevice();
		AudioDevice(AudioDevice &&other) noexcept;
		AudioDevice &operator=(AudioDevice &&other) noexcept;
		AudioDevice(const AudioDevice &other) noexcept = delete;
		AudioDevice &operator=(const AudioDevice &other) noexcept = delete;
	};
}
